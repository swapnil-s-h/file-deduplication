#include "zip_util.h"

// Stubs — real implementations require minizip or similar.
bool zip_read_file(const std::string& zipPath, const std::string& innerPath, std::string& out) {
    (void)zipPath; (void)innerPath; (void)out;
    return false;
}
bool zip_write_file_replace(const std::string& zipPath, const std::string& innerPath, const std::string& content) {
    (void)zipPath; (void)innerPath; (void)content;
    return false;
}
std::vector<std::string> zip_list_files(const std::string& zipPath) {
    (void)zipPath;
    return {};
}
