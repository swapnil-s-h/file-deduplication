#pragma once
#include <filesystem>
#include <string>

/// Remove duplicate rows (exact cell text match) in sheet1 of an .xlsx.
/// If commit=false, only analyze and fill report; no writeback.
/// Returns true if file would change (or did change when commit=true).
bool xlsx_dedupe_rows_inplace(const std::filesystem::path& p, bool commit, std::string& report);
