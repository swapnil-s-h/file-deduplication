#include "file_ops.h"
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <algorithm>

static bool has_ext(const std::filesystem::path& p, const std::vector<std::string>& whitelist) {
    if (whitelist.empty()) return true;
    auto e = p.extension().string();
    std::transform(e.begin(), e.end(), e.begin(), ::tolower);
    for (auto w : whitelist) {
        std::transform(w.begin(), w.end(), w.begin(), ::tolower);
        if (e == w) return true;
    }
    return false;
}

std::vector<FileInfo> list_target_files(const std::filesystem::path& dir, bool recurse,
                                        const std::vector<std::string>& only_exts) {
    std::vector<FileInfo> out;
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) return out;

    if (recurse) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
            if (ec) break;
            if (!entry.is_regular_file()) continue;
            if (!has_ext(entry.path(), only_exts)) continue;
            out.push_back({entry.path(), (std::uintmax_t)entry.file_size()});
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (ec) break;
            if (!entry.is_regular_file()) continue;
            if (!has_ext(entry.path(), only_exts)) continue;
            out.push_back({entry.path(), (std::uintmax_t)entry.file_size()});
        }
    }
    return out;
}

bool delete_file(const std::filesystem::path& p) {
    std::error_code ec;
    return std::filesystem::remove(p, ec);
}

// === TXT in-file dedup: keep first occurrence of each line ===
bool dedupe_txt_inplace(const std::filesystem::path& p) {
    std::ifstream in(p);
    if (!in) return false;
    std::vector<std::string> lines;
    std::unordered_set<std::string> seen;
    std::string s;
    while (std::getline(in, s)) {
        if (seen.insert(s).second) lines.push_back(std::move(s));
    }
    in.close();

    std::ifstream check(p, std::ios::binary);
    std::string original((std::istreambuf_iterator<char>(check)), {});
    std::string rebuilt;
    for (size_t i = 0; i < lines.size(); ++i) {
        rebuilt += lines[i];
        if (i + 1 < lines.size()) rebuilt += "\n";
    }
    if (original == rebuilt) return false; // no change

    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    out.write(rebuilt.data(), rebuilt.size());
    return true;
}
