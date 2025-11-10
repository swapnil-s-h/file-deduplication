#pragma once
#include <string>
#include <vector>
#include <filesystem>

struct FileInfo {
    std::filesystem::path path;
    std::uintmax_t size;
};

/// List files in 'dir'. If recurse==true, walk subfolders. If allow_ext not empty,
/// only files whose extension (lowercase, with dot) is in allow_ext are returned.
std::vector<FileInfo> list_files(const std::filesystem::path& dir, bool recurse = false, const std::vector<std::string>& allow_ext = {});

/// Delete a file (returns true on success).
bool delete_file(const std::filesystem::path& p);
