#include "xlsx_dedup.h"
#include "zip_util.h"
#include "hasher.h"
#include <tinyxml2.h>
#include <unordered_set>
#include <sstream>

using namespace tinyxml2;

// NOTE: This minimal implementation targets xl/worksheets/sheet1.xml,
// and dedupes identical rows by the concatenation of all <v> values.
// For production, you'd resolve sharedStrings & data types.

static std::string row_fingerprint(tinyxml2::XMLElement* row) {
    // concat inner text of all <v> nodes inside the row
    std::string s;
    for (auto* c = row->FirstChildElement(); c; c = c->NextSiblingElement()) {
        for (auto* v = c->FirstChildElement("v"); v; v = v->NextSiblingElement("v")) {
            if (const char* t = v->GetText()) { s += t; s.push_back('|'); }
        }
    }
    return sha256_hex(s);
}

bool xlsx_dedupe_rows_inplace(const std::filesystem::path& xlsx, bool commit, std::string& report) {
    std::string xml;
    if (!zip_read_file(xlsx.string(), "xl/worksheets/sheet1.xml", xml)) {
        report += "  [WARN] Unable to open xl/worksheets/sheet1.xml — skipping.\n";
        return false;
    }

    tinyxml2::XMLDocument d;
    if (d.Parse(xml.c_str(), xml.size()) != XML_SUCCESS) {
        report += "  [WARN] XML parse failed — skipping.\n";
        return false;
    }

    auto* ws = d.RootElement();         // <worksheet ...>
    if (!ws) { report += "  [WARN] No worksheet root.\n"; return false; }
    auto* sheetData = ws->FirstChildElement("sheetData");
    if (!sheetData) { report += "  [WARN] No sheetData.\n"; return false; }

    std::unordered_set<std::string> seen;
    std::vector<tinyxml2::XMLElement*> toDelete;

    for (auto* row = sheetData->FirstChildElement("row"); row; row = row->NextSiblingElement("row")) {
        std::string fp = row_fingerprint(row);
        if (!fp.empty()) {
            if (!seen.insert(fp).second) toDelete.push_back(row);
        }
    }

    std::ostringstream oss;
    oss << "    rows total=" << (int)seen.size() + (int)toDelete.size()
        << ", removed=" << (int)toDelete.size() << "\n";
    report += oss.str();

    if (toDelete.empty()) return false;

    if (commit) {
        for (auto* r : toDelete) sheetData->DeleteChild(r);
        tinyxml2::XMLPrinter pr;
        d.Print(&pr);
        if (!zip_write_file_replace(xlsx.string(), "xl/worksheets/sheet1.xml", pr.CStr())) {
            report += "  [ERR] Failed to write sheet1.xml back.\n";
            return false;
        }
    }
    return true;
}
