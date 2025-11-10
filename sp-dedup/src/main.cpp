#include "file_ops.h"
#include "hasher.h"
#include "docx_dedupe.h"
#include "xlsx_dedupe.h"

#include <filesystem>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout <<
            "sp-dedup\n"
            "Usage:\n"
            "  sp_dedup <folder> [--recurse] [--only-ext=.docx,.xlsx,.txt]\n"
            "  sp_dedup <folder> --purge        (delete duplicates, keep one)\n";
        return 0;
    }

    std::filesystem::path root = argv[1];
    bool recurse = false;
    bool purge = false;
    std::vector<std::string> allow_ext;

    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--recurse") recurse = true;
        else if (a == "--purge") purge = true;
        else if (a.rfind("--only-ext=", 0) == 0) {
            std::string rest = a.substr(11);
            size_t pos = 0;
            while (pos < rest.size()) {
                size_t comma = rest.find(',', pos);
                std::string ext = rest.substr(pos, comma == std::string::npos ? rest.size()-pos : comma - pos);
                if (!ext.empty()) {
                    if (ext[0] != '.') ext = "." + ext;
                    for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
                    allow_ext.push_back(ext);
                }
                if (comma == std::string::npos) break;
                pos = comma + 1;
            }
        } else {
            std::cerr << "Unknown option: " << a << "\n";
            return 1;
        }
    }

    auto files = list_files(root, recurse, allow_ext);

    using Key1 = std::pair<std::string, std::uintmax_t>;
    std::map<Key1, std::vector<FileInfo>> groups;
    for (auto& f : files) {
        groups[{std::string(f.path.extension().string()), f.size}].push_back(f);
    }

    std::size_t dup_sets = 0, dup_files = 0, total = files.size();
    for (auto& [k, vec] : groups) {
        if (vec.size() < 2) continue;
        std::unordered_map<std::string, std::vector<std::filesystem::path>> hmap;

        for (auto& fi : vec) {
            try {
                auto hex = sha256_hex_file(fi.path);
                hmap[hex].push_back(fi.path);
            } catch (const std::exception& e) {
                std::cerr << "Hash failed: " << fi.path << " : " << e.what() << "\n";
            }
        }

        for (auto& [hex, paths] : hmap) {
            if (paths.size() < 2) continue;
            ++dup_sets;
            dup_files += paths.size() - 1;

            std::cout << "\nDuplicate set (ext=" << k.first << ", size=" << k.second
                      << ", sha256=" << hex << ")\n";
            for (size_t i = 0; i < paths.size(); ++i) {
                std::cout << "  [" << (i==0 ? "KEEP" : "DEL ") << "] " << paths[i].string() << "\n";
            }

            if (purge) {
                for (size_t i = 1; i < paths.size(); ++i) {
                    if (delete_file(paths[i])) {
                        std::cout << "     deleted: " << paths[i].string() << "\n";
                    }
                }
            }
        }
    }

    std::cout << "\nScanned files: " << total
              << "\nDuplicate sets: " << dup_sets
              << "\nFiles removable: " << dup_files << "\n";

    // Phase 2: inside-file dedupe (stubs or real depending on libs)
    for (auto const& f : files) {
        auto ext = f.path.extension().string();
        for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
        if (ext == ".docx") {
            if (docx_remove_duplicate_paragraphs(f.path))
                std::cout << "[DOCX] Cleaned duplicate paragraphs: " << f.path << "\n";
        } else if (ext == ".xlsx") {
            if (xlsx_remove_duplicate_rows(f.path))
                std::cout << "[XLSX] Cleaned duplicate rows: " << f.path << "\n";
        }
    }

    return 0;
}
