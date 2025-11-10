#include "xlsx_dedupe.h"

#ifdef HAS_DOCX_XLSX
#include <iostream>
bool xlsx_remove_duplicate_rows(const std::filesystem::path& p) {
    std::cout << "[XLSX] Real implementation would run here on: " << p << "\n";
    // TODO: implement using minizip + tinyxml2
    return false;
}
#else
bool xlsx_remove_duplicate_rows(const std::filesystem::path& p) {
    (void)p;
    return false;
}
#endif
