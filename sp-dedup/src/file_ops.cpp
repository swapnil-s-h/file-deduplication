#include "file_ops.h"
#include <algorithm>
#include <iostream>
#include <cctype>

static std::string norm_ext(const std::filesystem::path& p) {
    std::string e = p.extension().string();
    std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return e;
}

std::vector<FileInfo> list_files(const std::filesystem::path& dir, bool recurse, const std::vector<std::string>& allow_ext) {
    std::vector<FileInfo> out;
    if (!std::filesystem::exists(dir)) return out;

    std::error_code ec;
    if (recurse) {
        for (auto const& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
            if (ec) { std::cerr << "Directory iteration error: " << ec.message() << "\n"; break; }
            if (!entry.is_regular_file()) continue;
            FileInfo fi;
            fi.path = entry.path();
            fi.size = entry.file_size();
            if (!allow_ext.empty()) {
                std::string e = norm_ext(fi.path);
                bool ok = false;
                for (auto const& a : allow_ext) if (e == a) { ok = true; break; }
                if (!ok) continue;
            }
            out.push_back(std::move(fi));
        }
    } else {
        for (auto const& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (ec) { std::cerr << "Directory iteration error: " << ec.message() << "\n"; break; }
            if (!entry.is_regular_file()) continue;
            FileInfo fi;
            fi.path = entry.path();
            fi.size = entry.file_size();
            if (!allow_ext.empty()) {
                std::string e = norm_ext(fi.path);
                bool ok = false;
                for (auto const& a : allow_ext) if (e == a) { ok = true; break; }
                if (!ok) continue;
            }
            out.push_back(std::move(fi));
        }
    }
    return out;
}

bool delete_file(const std::filesystem::path& p) {
    std::error_code ec;
    bool res = std::filesystem::remove(p, ec);
    if (!res && ec) {
        std::cerr << "Failed to delete " << p << " : " << ec.message() << "\n";
    }
    return res;
}
