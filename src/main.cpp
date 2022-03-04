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
#include <iostream>
#include "uring.hpp"
#include <type_traits>

int main(int argc, char *argv[]) {
    using std::cout, std::endl;
    
    cout << "1:\n";
    {
        liburingcxx::URing ring{8, 0};
    }
    cout << "2:\n";

    {
        using namespace liburingcxx;
        using namespace detail;   
        static_assert(std::is_standard_layout_v<URing>);
        static_assert(!std::is_pod_v<URing>);
        static_assert(std::is_pod_v<SQEntry>);
        static_assert(std::is_pod_v<CQEntry>);
        static_assert(std::is_pod_v<SubmissionQueue>);
        static_assert(std::is_pod_v<CompletionQueue>);
    }

    return 0;
}