#pragma once
#include <string>
#include <filesystem>

/// Compute SHA256 of in-memory bytes (hex string).
std::string sha256_hex(const std::string& bytes);

/// Compute SHA256 of a file and return hex string. Throws std::runtime_error on I/O error.
std::string sha256_hex_file(const std::filesystem::path& p);
