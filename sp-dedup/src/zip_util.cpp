#include "zip_util.h"
#include <minizip/zip.h>
#include <minizip/unzip.h>
#include <cstdio>
#include <vector>
#include <filesystem>
#include <stdexcept>

static bool read_whole_file(unzFile uf, std::string& out) {
    char buf[1 << 15];
    int rd = 0;
    out.clear();
    for (;;) {
        rd = unzReadCurrentFile(uf, buf, sizeof(buf));
        if (rd < 0) return false;
        if (rd == 0) break;
        out.append(buf, buf + rd);
    }
    return true;
}

bool zip_read_file(const std::string& zipPath, const std::string& innerPath, std::string& out) {
    unzFile uf = unzOpen64(zipPath.c_str());
    if (!uf) return false;
    bool ok = false;
    if (UNZ_OK == unzLocateFile(uf, innerPath.c_str(), 2) &&
        UNZ_OK == unzOpenCurrentFile(uf)) {
        ok = read_whole_file(uf, out);
        unzCloseCurrentFile(uf);
    }
    unzClose(uf);
    return ok;
}

std::vector<std::string> zip_list_files(const std::string& zipPath) {
    std::vector<std::string> files;
    unzFile uf = unzOpen64(zipPath.c_str());
    if (!uf) return files;
    if (UNZ_OK != unzGoToFirstFile(uf)) { unzClose(uf); return files; }
    do {
        char name[512]; unz_file_info64 info{};
        if (UNZ_OK == unzGetCurrentFileInfo64(uf, &info, name, sizeof(name), nullptr, 0, nullptr, 0)) {
            files.emplace_back(name);
        }
    } while (UNZ_OK == unzGoToNextFile(uf));
    unzClose(uf);
    return files;
}

// Replace one entry by rebuilding the archive to a temp file (simple & safe).
bool zip_write_file_replace(const std::string& zipPath, const std::string& innerPath, const std::string& content) {
    auto tmp = zipPath + ".tmp";
    unzFile in = unzOpen64(zipPath.c_str());
    if (!in) return false;

    zipFile out = zipOpen64(tmp.c_str(), 0);
    if (!out) { unzClose(in); return false; }

    auto copy_current_file = [&](const char* fname) -> bool {
        if (UNZ_OK != unzOpenCurrentFile(in)) return false;
        zip_fileinfo zi{}; // default times
        if (ZIP_OK != zipOpenNewFileInZip64(out, fname, &zi,
                nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 1))
            return false;

        char buf[1<<15];
        for (;;) {
            int rd = unzReadCurrentFile(in, buf, sizeof(buf));
            if (rd < 0) return false;
            if (rd == 0) break;
            if (ZIP_OK != zipWriteInFileInZip(out, buf, rd)) return false;
        }
        unzCloseCurrentFile(in);
        zipCloseFileInZip(out);
        return true;
    };

    // iterate all files, copy except the one we replace
    if (UNZ_OK != unzGoToFirstFile(in)) { zipClose(out, nullptr); unzClose(in); return false; }
    do {
        char name[512]; unz_file_info64 info{};
        if (UNZ_OK != unzGetCurrentFileInfo64(in, &info, name, sizeof(name), nullptr, 0, nullptr, 0)) { zipClose(out, nullptr); unzClose(in); return false; }
        if (innerPath == name) {
            // write replacement
            zip_fileinfo zi{};
            if (ZIP_OK != zipOpenNewFileInZip64(out, name, &zi, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 1)) { zipClose(out, nullptr); unzClose(in); return false; }
            if (!content.empty() && ZIP_OK != zipWriteInFileInZip(out, content.data(), (unsigned)content.size())) { zipCloseFileInZip(out); zipClose(out, nullptr); unzClose(in); return false; }
            zipCloseFileInZip(out);
        } else {
            if (!copy_current_file(name)) { zipClose(out, nullptr); unzClose(in); return false; }
        }
    } while (UNZ_OK == unzGoToNextFile(in));

    zipClose(out, nullptr);
    unzClose(in);

    // replace original
    std::error_code ec;
    std::filesystem::remove(zipPath, ec);
    std::filesystem::rename(tmp, zipPath, ec);
    return !ec;
}
