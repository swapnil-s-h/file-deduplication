#include "docx_dedupe.h"

#ifdef HAS_DOCX_XLSX
#include <iostream>

bool docx_remove_duplicate_paragraphs(const std::filesystem::path& p) {
    std::cout << "[DOCX] Real implementation would run here on: " << p << "\n";
    // TODO: implement using minizip + tinyxml2
    return false;
}

#else

bool docx_remove_duplicate_paragraphs(const std::filesystem::path& p) {
    (void)p;
    // stubs active when tinyxml2/minizip not available
    return false;
}

#endif
