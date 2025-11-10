#include "hasher.h"
#include <fstream>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

std::string sha256_hex_file(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open file for hashing: " + p.string());

    // Open algorithm provider
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0))) {
        throw std::runtime_error("BCryptOpenAlgorithmProvider failed");
    }

    // Create hash object
    BCRYPT_HASH_HANDLE hHash = nullptr;
    if (!BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0))) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        throw std::runtime_error("BCryptCreateHash failed");
    }

    std::vector<char> buff(4096);
    while (in) {
        in.read(buff.data(), (std::streamsize)buff.size());
        std::streamsize got = in.gcount();
        if (got > 0) {
            if (!BCRYPT_SUCCESS(BCryptHashData(hHash, reinterpret_cast<PUCHAR>(buff.data()), (ULONG)got, 0))) {
                BCryptDestroyHash(hHash);
                BCryptCloseAlgorithmProvider(hAlg, 0);
                throw std::runtime_error("BCryptHashData failed");
            }
        }
    }

    // SHA-256 length is 32 bytes
    const ULONG hash_len = 32;
    std::vector<unsigned char> hash(hash_len);
    if (!BCRYPT_SUCCESS(BCryptFinishHash(hHash, hash.data(), hash_len, 0))) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        throw std::runtime_error("BCryptFinishHash failed");
    }

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char b : hash) oss << std::setw(2) << static_cast<int>(b);
    return oss.str();
}
