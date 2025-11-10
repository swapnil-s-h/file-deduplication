#pragma once
#include <string>
#include <vector>

bool zip_read_file(const std::string& zipPath, const std::string& innerPath, std::string& out);
bool zip_write_file_replace(const std::string& zipPath, const std::string& innerPath, const std::string& content);
std::vector<std::string> zip_list_files(const std::string& zipPath);
