#include "docx_dedup.h"
#include "zip_util.h"
#include "hasher.h"
#include <tinyxml2.h>
#include <unordered_set>
#include <sstream>

using namespace tinyxml2;

static std::string get_text_concat(XMLNode* node) {
    // DOCX text nodes are w:t; they can be nested inside w:r inside w:p
    std::string out;
    for (auto* e = node->FirstChildElement(); e; e = e->NextSiblingElement()) {
        const char* name = e->Name(); // e.g., "w:r", "w:t"
        if (name && std::string(name).find(":t") != std::string::npos) {
            if (const char* t = e->GetText()) out += t;
        }
        out += get_text_concat(e);
    }
    return out;
}

bool docx_dedupe_paragraphs_inplace(const std::filesystem::path& docx, bool commit, std::string& report) {
    std::string xml;
    if (!zip_read_file(docx.string(), "word/document.xml", xml)) {
        report += "  [WARN] Unable to open word/document.xml — skipping.\n";
        return false;
    }

    XMLDocument d;
    if (d.Parse(xml.c_str(), xml.size()) != XML_SUCCESS) {
        report += "  [WARN] XML parse failed — skipping.\n";
        return false;
    }

    // paragraphs are w:p
    std::unordered_set<std::string> seen;
    std::vector<XMLElement*> toDelete;

    for (XMLElement* p = d.RootElement()->FirstChildElement(); p; p = p->NextSiblingElement()) {
        const char* nm = p->Name();
        if (!nm || std::string(nm).find(":p") == std::string::npos) continue;
        std::string text = get_text_concat(p);
        // Normalize whitespace a bit
        if (!text.empty()) {
            // hash the text to decide duplicates
            std::string h = sha256_hex(text);
            if (!seen.insert(h).second) {
                toDelete.push_back(p);
            }
        }
    }

    std::ostringstream oss;
    oss << "    paragraphs total=" << (int)seen.size() + (int)toDelete.size()
        << ", removed=" << (int)toDelete.size() << "\n";
    report += oss.str();

    if (toDelete.empty()) return false;

    if (commit) {
        for (auto* p : toDelete) p->Parent()->DeleteChild(p);
        XMLPrinter pr;
        d.Print(&pr);
        if (!zip_write_file_replace(docx.string(), "word/document.xml", pr.CStr())) {
            report += "  [ERR] Failed to write document.xml back.\n";
            return false;
        }
    }
    return true;
}
