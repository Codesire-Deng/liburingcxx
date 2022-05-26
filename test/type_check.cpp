/*
 *  A type tester of liburingcxx.
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
#include <iostream>
#include "uring/uring.hpp"
#include <type_traits>
#include <liburing.h>

int main(int argc, char *argv[]) {
    using std::cout, std::endl;

    using namespace liburingcxx;
    using namespace detail;
    using namespace std;
    static_assert(is_standard_layout_v<URing<0>> && !is_trivial_v<URing<0>>);
    static_assert(is_standard_layout_v<SQEntry> && is_trivial_v<SQEntry>);
    static_assert(is_standard_layout_v<CQEntry> && is_trivial_v<CQEntry>);
    static_assert(is_standard_layout_v<SubmissionQueue> && is_trivial_v<SubmissionQueue>);
    static_assert(is_standard_layout_v<CompletionQueue> && is_trivial_v<CompletionQueue>);

    static_assert(sizeof(io_uring_sqe) == sizeof(SQEntry));
    static_assert(sizeof(io_uring_cqe) == sizeof(CQEntry));
    // static_assert(sizeof(URing) == sizeof(io_uring));
    static_assert(sizeof(io_uring_sq) != sizeof(SubmissionQueue));
    static_assert(sizeof(io_uring_cq) != sizeof(CompletionQueue));

    cout << "All test passed!\n";

    return 0;
}