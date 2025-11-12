#pragma once
#include <string>
#include <vector>
#include <filesystem>

struct FileInfo {
    std::filesystem::path path;
    std::uintmax_t size{};
};

std::vector<FileInfo> list_target_files(const std::filesystem::path& dir, bool recurse,
                                        const std::vector<std::string>& only_exts);

bool delete_file(const std::filesystem::path& p);

// Phase 2 helpers
bool dedupe_txt_inplace(const std::filesystem::path& p);
