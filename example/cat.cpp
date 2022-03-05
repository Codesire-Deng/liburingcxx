/*
 *  A tester using liburingcxx.
 *
 *  Copyright (C) 2022 Zifeng Deng
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <fcntl.h>
// #include <sys/stat.h>
#include <iostream>
#include <filesystem>
#include "uring.hpp"

constexpr unsigned BLOCK_SZ = 1024;

struct FileInfo {
    size_t size;
    iovec iovecs[];
};

void submitReadRequest(
    liburingcxx::URing &ring, const std::filesystem::path path) {
    int file_fd = open(path.c_str(), O_RDONLY);
    if (file_fd < 0) {
        throw std::system_error{errno, std::system_category(), "open"};
    }
    size_t file_size = std::filesystem::file_size(path);
    const unsigned blocks =
        file_size / BLOCK_SZ + ((file_size / BLOCK_SZ * BLOCK_SZ) != file_size);
    FileInfo *fi = (FileInfo *)malloc(sizeof(*fi) + (blocks * sizeof(iovec)));
    fi->size = file_size;

    for (size_t rest = file_size, offset = 0, i = 0; rest != 0; ++i) {
        size_t to_read = std::min<size_t>(rest, BLOCK_SZ);
        fi->iovecs[i].iov_len = to_read;
        if (posix_memalign(&fi->iovecs[i].iov_base, BLOCK_SZ, BLOCK_SZ) != 0) {
            throw std::system_error{
                errno, std::system_category(), "posix_memalign"};
        }
        rest -= to_read;
    }

    liburingcxx::SQEntry &sqe = *ring.getSQEntry();
    sqe.prepareReadv(file_fd, std::span{fi->iovecs, blocks}, 0)
        .setData(reinterpret_cast<uint64_t>(fi));
    ring.submit();
}

void output(std::string_view s) {
    for (char c : s)
        putchar(c);
}

void waitResultAndPrint(liburingcxx::URing &ring) {
    liburingcxx::CQEntry &cqe = *ring.waitCQEntry();
    FileInfo *fi = reinterpret_cast<FileInfo *>(cqe.getData());
    const int blocks =
        fi->size / BLOCK_SZ + ((fi->size / BLOCK_SZ * BLOCK_SZ) != fi->size);
    for (int i=0; i<blocks; ++i) {
        output({(char*)fi->iovecs[i].iov_base, fi->iovecs[i].iov_len});
    }

    ring.SeenCQEntry(&cqe);
}

int main(int argc, char *argv[]) {
    using std::cout, std::endl, std::cerr;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s [file name] <[file name] ...>\n", argv[0]);
        return 1;
    }

    liburingcxx::URing ring{4, 0};

    for (int i = 1; i < argc; ++i) {
        try {
            submitReadRequest(ring, argv[i]);
            waitResultAndPrint(ring);
            /* code */
        } catch (const std::system_error &e) {
            cerr << e.what() << "\n" << e.code() << "\n";
        } catch (const std::exception &e) { cerr << e.what() << '\n'; }
    }

    return 0;
}