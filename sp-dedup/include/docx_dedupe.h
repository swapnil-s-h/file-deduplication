#pragma once
#include <filesystem>

/// Returns true if modifications were made (i.e., duplicate paragraphs removed)
bool docx_remove_duplicate_paragraphs(const std::filesystem::path& p);
