#pragma once
#include <filesystem>
#include <string>

/// Remove duplicate paragraphs (exact text match) inside a .docx.
/// If commit=false, only analyze and fill report; no writeback.
/// Returns true if the file would change (or did change when commit=true).
bool docx_dedupe_paragraphs_inplace(const std::filesystem::path& p, bool commit, std::string& report);
