#include "hasher.h"
#include <stdexcept>
#include <vector>
#include <windows.h>
#include <bcrypt.h>
#include <fstream>

#pragma comment(lib, "bcrypt.lib")

namespace {
    std::string to_hex(const std::vector<unsigned char>& buf) {
        static const char* hex = "0123456789abcdef";
        std::string out; out.resize(buf.size()*2);
        for (size_t i=0;i<buf.size();++i){ out[2*i]=hex[(buf[i]>>4)&0xF]; out[2*i+1]=hex[buf[i]&0xF]; }
        return out;
    }

    std::string sha256_raw(const unsigned char* data, size_t len) {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_HASH_HANDLE hHash = nullptr;
        NTSTATUS s = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
        if (s < 0) throw std::runtime_error("BCryptOpenAlgorithmProvider failed");

        DWORD objLen=0, cb=0;
        s = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objLen, sizeof(objLen), &cb, 0);
        if (s < 0) { BCryptCloseAlgorithmProvider(hAlg,0); throw std::runtime_error("Get obj len failed"); }

        DWORD hashLen=0;
        s = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PUCHAR)&hashLen, sizeof(hashLen), &cb, 0);
        if (s < 0) { BCryptCloseAlgorithmProvider(hAlg,0); throw std::runtime_error("Get hash len failed"); }

        std::vector<BYTE> obj(objLen);
        std::vector<unsigned char> hash(hashLen);

        s = BCryptCreateHash(hAlg, &hHash, obj.data(), objLen, nullptr, 0, 0);
        if (s < 0) { BCryptCloseAlgorithmProvider(hAlg,0); throw std::runtime_error("CreateHash failed"); }

        if (len) {
            s = BCryptHashData(hHash, (PUCHAR)data, (ULONG)len, 0);
            if (s < 0) { BCryptDestroyHash(hHash); BCryptCloseAlgorithmProvider(hAlg,0); throw std::runtime_error("HashData failed"); }
        }

        s = BCryptFinishHash(hHash, hash.data(), (ULONG)hash.size(), 0);
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg,0);
        if (s < 0) throw std::runtime_error("FinishHash failed");

        return to_hex(hash);
    }
}

std::string sha256_hex(const std::string& bytes) {
    return sha256_raw(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
}

std::string sha256_hex_file(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("Failed to open file for hashing: " + p.string());
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    NTSTATUS s = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (s < 0) throw std::runtime_error("BCryptOpenAlgorithmProvider failed");

    DWORD objLen=0, cb=0;
    s = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objLen, sizeof(objLen), &cb, 0);
    if (s < 0) { BCryptCloseAlgorithmProvider(hAlg,0); throw std::runtime_error("Get obj len failed"); }

    std::vector<BYTE> obj(objLen);
    DWORD hashLen=0;
    s = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PUCHAR)&hashLen, sizeof(hashLen), &cb, 0);
    if (s < 0) { BCryptCloseAlgorithmProvider(hAlg,0); throw std::runtime_error("Get hash len failed"); }
    std::vector<unsigned char> hash(hashLen);

    s = BCryptCreateHash(hAlg, &hHash, obj.data(), objLen, nullptr, 0, 0);
    if (s < 0) { BCryptCloseAlgorithmProvider(hAlg,0); throw std::runtime_error("CreateHash failed"); }

    std::vector<char> buf(1<<16);
    while (f) {
        f.read(buf.data(), buf.size());
        std::streamsize got = f.gcount();
        if (got>0) {
            s = BCryptHashData(hHash, (PUCHAR)buf.data(), (ULONG)got, 0);
            if (s < 0) { BCryptDestroyHash(hHash); BCryptCloseAlgorithmProvider(hAlg,0); throw std::runtime_error("HashData failed"); }
        }
    }
    s = BCryptFinishHash(hHash, hash.data(), (ULONG)hash.size(), 0);
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg,0);
    if (s < 0) throw std::runtime_error("FinishHash failed");
    // hex
    static const char* hex = "0123456789abcdef";
    std::string out; out.resize(hash.size()*2);
    for (size_t i=0;i<hash.size();++i){ out[2*i]=hex[(hash[i]>>4)&0xF]; out[2*i+1]=hex[hash[i]&0xF]; }
    return out;
}
