#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <string>
#include <optional>
#include "file_ops.h"
#include "hasher.h"
#include "docx_dedup.h"
#include "xlsx_dedup.h"

namespace fs = std::filesystem;

struct Args {
    fs::path root;
    bool recurse = false;
    std::unordered_set<std::string> only_ext;   // e.g. {".docx",".xlsx",".txt"}
    bool commit = false;        // actually delete / rewrite
    bool within = false;        // Phase-2 in-file dedup
};

static void usage() {
    std::cout <<
        "Usage:\n"
        "  sp_dedup.exe <directory> [--recurse] [--only-ext=.docx,.xlsx,.txt]\n"
        "               [--commit] [--within]\n"
        "Examples:\n"
        "  sp_dedup.exe D:\\Documents\\sample_files --recurse --only-ext=.docx,.xlsx,.txt\n"
        "  sp_dedup.exe D:\\docs --recurse --only-ext=.docx --within --commit\n";
}

static std::optional<Args> parse(int argc, char** argv) {
    if (argc < 2) { usage(); return std::nullopt; }
    Args a; a.root = fs::path(argv[1]);
    for (int i=2;i<argc;i++) {
        std::string s = argv[i];
        if (s == "--recurse") a.recurse = true;
        else if (s.rfind("--only-ext=",0)==0) {
            std::string list = s.substr(std::string("--only-ext=").size());
            size_t pos=0;
            while (pos < list.size()) {
                size_t comma = list.find(',', pos);
                auto item = list.substr(pos, comma==std::string::npos ? std::string::npos : comma-pos);
                if (!item.empty()) a.only_ext.insert(item);
                if (comma==std::string::npos) break;
                pos = comma+1;
            }
        } else if (s == "--commit") a.commit = true;
        else if (s == "--within") a.within = true;
        else { std::cerr << "Unknown arg: " << s << "\n"; usage(); return std::nullopt; }
    }
    return a;
}

int main(int argc, char** argv) {
    auto argsOpt = parse(argc, argv);
    if (!argsOpt) return 1;
    auto args = *argsOpt;

    if (!fs::exists(args.root) || !fs::is_directory(args.root)) {
        std::cerr << "Not a directory: " << args.root << "\n";
        return 2;
    }

    // Phase-1: file-level duplicate removal (hash-bytes, group by ext+size+sha256)
    std::vector<std::string> ext_filter;
    if (!args.only_ext.empty()) {
        for (auto& e : args.only_ext) ext_filter.push_back(e);
    }

    auto files = list_target_files(args.root, args.recurse, ext_filter);

    std::unordered_map<std::string, std::vector<fs::path>> buckets; // key=ext|size|sha
    size_t scanned=0;

    for (auto& fi : files) {
        if (!args.only_ext.empty()) {
            auto ext = fi.path.extension().string();
            if (args.only_ext.count(ext)==0) continue;
        }
        try {
            auto h = sha256_hex_file(fi.path);
            std::string key = fi.path.extension().string() + "|" + std::to_string(fi.size) + "|" + h;
            buckets[key].push_back(fi.path);
            ++scanned;
        } catch (...) {
            std::cerr << "Failed to hash: " << fi.path << "\n";
        }
    }

    size_t dupSets=0, removable=0;
    for (auto& [key, vec] : buckets) {
        if (vec.size() < 2) continue;
        ++dupSets;
        //stable keep-first
        std::cout << "\nDuplicate set (ext=" << fs::path(vec[0]).extension().string()
                  << ", size=" << fs::file_size(vec[0])
                  << ", sha256=" << key.substr(key.rfind('|')+1) << ")\n";
        for (size_t i=0;i<vec.size();++i) {
            if (i==0) {
                std::cout << "  [KEEP] " << vec[i].string() << "\n";
            } else {
                std::cout << "  [DEL ] " << vec[i].string() << "\n";
                if (args.commit) delete_file(vec[i]);
                ++removable;
            }
        }
    }

    std::cout << "\nScanned files: " << scanned << "\n"
              << "Duplicate sets: " << dupSets << "\n"
              << "Files removable: " << removable << "\n";

    // Phase-2: within-file dedup (docx/xlsx/txt)
    if (args.within) {
        std::cout << "\n=== Phase-2: Within-file de-duplication ===\n";
        for (auto& fi : files) {
            if (!args.only_ext.empty() && args.only_ext.count(fi.path.extension().string())==0) continue;

            std::string report;
            bool changed = false;
            try {
                auto ext = fi.path.extension().string();
                if (ext == ".docx") {
                    changed = docx_dedupe_paragraphs_inplace(fi.path, args.commit, report);
                } else if (ext == ".xlsx") {
                    changed = xlsx_dedupe_rows_inplace(fi.path, args.commit, report);
                } else if (ext == ".txt") {
                    // optional: simple line de-dup (keep first occurrence)
                    // read, fingerprint lines, rewrite if needed
                    // (left as-is to keep focus on docx/xlsx)
                }
            } catch (const std::exception& e) {
                report += std::string("  [ERR] ") + e.what() + "\n";
            }
            if (!report.empty()) {
                std::cout << fi.path.string() << "\n" << report;
                if (changed) std::cout << (args.commit ? "  [WROTE]\n" : "  [WOULD WRITE]\n");
            }
        }
    }

    if (!args.commit) {
        std::cout << "\nNOTE: dry-run mode. Use --commit to apply deletions/rewrites.\n";
    }
    return 0;
}
