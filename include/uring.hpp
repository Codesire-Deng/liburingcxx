/*
 *  A C++ helper for io_uring
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
#pragma once

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500 /* Required for glibc to expose sigset_t */
#endif

#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cerrno>
#include <signal.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <sched.h>
#include <linux/swab.h>
#include <system_error>
#include <cstring>
#include "liburing/compat.h"
#include "liburing/io_uring.h"
#include "liburing/barrier.h"
#include "uring/syscall.hpp"

namespace liburingcxx {

class URing;

class SQEntry : private io_uring_sqe {
  public:
    friend class ::liburingcxx::URing;

    inline SQEntry &setData(uint64_t data) noexcept {
        this->user_data = data;
        return *this;
    }

    inline SQEntry &setFlags(uint8_t flags) noexcept {
        this->flags = flags;
        return *this;
    }

    inline SQEntry &setTargetFixedFile(uint32_t fileIndex) noexcept {
        /* 0 means no fixed files, indexes should be encoded as "index + 1" */
        this->file_index = fileIndex + 1;
        return *this;
    }

    inline SQEntry &prepareRW(
        uint8_t op,
        int fd,
        const void *addr,
        uint32_t len,
        uint64_t offset) noexcept {
        this->opcode = op;
        this->flags = 0;
        this->ioprio = 0;
        this->fd = fd;
        this->off = offset;
        this->addr = reinterpret_cast<uint64_t>(addr);
        this->len = len;
        this->rw_flags = 0;
        this->user_data = 0;
        this->buf_index = 0;
        this->personality = 0;
        this->file_index = 0;
        this->__pad2[0] = this->__pad2[1] = 0;
        return *this;
    }

    // TODO: more prepare
};

class CQEntry : private io_uring_cqe {
  public:
    friend class ::liburingcxx::URing;
    inline uint64_t getData() const noexcept { return this->user_data; }
};

namespace detail {

    struct URingParams : io_uring_params {
        /**
         * @brief Construct a new io_uring_params without initializing
         */
        URingParams() noexcept = default;

        /**
         * @brief Construct a new io_uring_params with memset and flags
         */
        explicit URingParams(unsigned flags) noexcept {
            memset(this, 0, sizeof(*this));
            this->flags = flags;
        }
    };

    class SubmissionQueue {
      private:
        unsigned *khead;
        unsigned *ktail;
        unsigned *kring_mask;
        unsigned *kring_entries;
        unsigned *kflags;
        unsigned *kdropped;
        unsigned *array;
        struct io_uring_sqe *sqes;

        unsigned sqe_head; // memset to 0 during URing()
        unsigned sqe_tail; // memset to 0 during URing()

        size_t ring_sz;
        void *ring_ptr;

        unsigned pad[4];

      private:
        void setOffset(const io_sqring_offsets &off) noexcept {
            khead = (unsigned *)((char *)ring_ptr + off.head);
            ktail = (unsigned *)((char *)ring_ptr + off.tail);
            kring_mask = (unsigned *)((char *)ring_ptr + off.ring_mask);
            kring_entries = (unsigned *)((char *)ring_ptr + off.ring_entries);
            kflags = (unsigned *)((char *)ring_ptr + off.flags);
            kdropped = (unsigned *)((char *)ring_ptr + off.dropped);
            array = (unsigned *)((char *)ring_ptr + off.array);
        }

        /**
         * @brief Sync internal state with kernel ring state on the SQ side.
         *
         * @return unsigned number of pending items in the SQ ring, for the
         * shared ring.
         */
        unsigned flush() noexcept {
            const unsigned mask = *kring_mask;
            unsigned tail = *ktail;
            unsigned to_submit = sqe_tail - sqe_head;
            if (to_submit == 0) return tail - *khead; // see below

            /*
             * Fill in sqes that we have queued up, adding them to the kernel
             * ring
             */
            do {
                array[tail & mask] = sqe_head & mask;
                tail++;
                sqe_head++;
            } while (--to_submit);

            /*
             * Ensure that the kernel sees the SQE updates before it sees the
             * tail update.
             */
            io_uring_smp_store_release(ktail, tail);

            /*
             * This _may_ look problematic, as we're not supposed to be reading
             * SQ->head without acquire semantics. When we're in SQPOLL mode,
             * the kernel submitter could be updating this right now. For
             * non-SQPOLL, task itself does it, and there's no potential race.
             * But even for SQPOLL, the load is going to be potentially
             * out-of-date the very instant it's done, regardless or whether or
             * not it's done atomically. Worst case, we're going to be
             * over-estimating what we can submit. The point is, we need to be
             * able to deal with this situation regardless of any perceived
             * atomicity.
             */
            return tail - *khead;
        }

      public:
        friend class ::liburingcxx::URing;
        SubmissionQueue() noexcept = default;
        ~SubmissionQueue() noexcept = default;
    };

    class CompletionQueue {
      private:
        unsigned *khead;
        unsigned *ktail;
        unsigned *kring_mask;
        unsigned *kring_entries;
        unsigned *kflags;
        unsigned *koverflow;
        struct io_uring_cqe *cqes;

        size_t ring_sz;
        void *ring_ptr;

        unsigned pad[4];

      private:
        void setOffset(const io_cqring_offsets &off) noexcept {
            khead = (unsigned *)((char *)ring_ptr + off.head);
            ktail = (unsigned *)((char *)ring_ptr + off.tail);
            kring_mask = (unsigned *)((char *)ring_ptr + off.ring_mask);
            kring_entries = (unsigned *)((char *)ring_ptr + off.ring_entries);
            if (off.flags) kflags = (unsigned *)((char *)ring_ptr + off.flags);
            koverflow = (unsigned *)((char *)ring_ptr + off.overflow);
            cqes = (io_uring_cqe *)((char *)ring_ptr + off.cqes);
        }

      public:
        friend class ::liburingcxx::URing;
        CompletionQueue() noexcept = default;
        ~CompletionQueue() noexcept = default;
    };

} // namespace detail

class [[nodiscard]] URing final {
  public:
    using Params = detail::URingParams;

  private:
    using SubmissionQueue = detail::SubmissionQueue;
    using CompletionQueue = detail::CompletionQueue;

    SubmissionQueue sq;
    CompletionQueue cq;
    unsigned flags;
    int ring_fd;

    unsigned features;
    unsigned pad[3];

  public:
    /**
     * @brief Submit sqes acquired from io_uring_get_sqe() to the kernel.
     *
     * @return unsigned number of sqes submitted
     */
    unsigned submit() {
        const unsigned submitted = sq.flush();
        unsigned enterFlags = 0;

        if (isSQRingNeedEnter(enterFlags)) {
            if ((this->flags & IORING_SETUP_IOPOLL))
                enterFlags |= IORING_ENTER_GETEVENTS;

            const int consumedNum = detail::__sys_io_uring_enter(
                ring_fd, submitted, 0, enterFlags, NULL);

            if (consumedNum < 0) [[unlikely]]
                throw std::system_error{
                    errno, std::system_category(), "submitAndWait"};
        }

        return submitted;
    }

    /**
     * @brief Submit sqes acquired from io_uring_get_sqe() to the kernel.
     *
     * @return unsigned number of sqes submitted
     */
    unsigned submitAndWait(unsigned waitNum) {
        const unsigned submitted = sq.flush();
        unsigned enterFlags = 0;

        if (waitNum || isSQRingNeedEnter(enterFlags)) {
            if (waitNum || (this->flags & IORING_SETUP_IOPOLL))
                enterFlags |= IORING_ENTER_GETEVENTS;

            const int consumedNum = detail::__sys_io_uring_enter(
                ring_fd, submitted, waitNum, enterFlags, NULL);

            if (consumedNum < 0) [[unlikely]]
                throw std::system_error{
                    errno, std::system_category(), "submitAndWait"};
        }

        return submitted;
    }

  public:
    URing(unsigned entries, Params &params) {
        const int fd = detail::__sys_io_uring_setup(entries, &params);
        if (fd < 0) [[unlikely]]
            throw std::system_error{
                errno, std::system_category(), "__sys_io_uring_setup"};

        memset(this, 0, sizeof(*this));
        this->flags = params.flags;
        this->ring_fd = fd;
        this->features = params.features;
        try {
            mmapQueue(fd, params);
        } catch (...) {
            close(fd);
            std::rethrow_exception(std::current_exception());
        }
    }

    URing(unsigned entries, Params &&params) : URing(entries, params) {}

    URing(unsigned entries, unsigned flags) : URing(entries, Params{flags}) {}

    /**
     * ban all copying or moving
     */
    URing(const URing &) = delete;
    URing(URing &&) = delete;
    URing &operator=(const URing &) = delete;
    URing &operator=(URing &&) = delete;

    ~URing() noexcept {
        munmap(sq.sqes, *sq.kring_entries * sizeof(io_uring_sqe));
        unmapRings();
        close(ring_fd);
    }

  private:
    /**
     * @brief Create mapping from kernel to SQ and CQ.
     *
     * @param fd fd of io_uring in kernel
     * @param p params describing the shape of ring
     */
    void mmapQueue(int fd, Params &p) {
        sq.ring_sz = p.sq_off.array + p.sq_entries * sizeof(unsigned);
        cq.ring_sz = p.cq_off.cqes + p.cq_entries * sizeof(io_uring_cqe);

        if (p.features & IORING_FEAT_SINGLE_MMAP)
            sq.ring_sz = cq.ring_sz = std::max(sq.ring_sz, cq.ring_sz);

        sq.ring_ptr = mmap(
            nullptr, sq.ring_sz, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_POPULATE, fd, IORING_OFF_SQ_RING);
        if (sq.ring_ptr == MAP_FAILED) [[unlikely]]
            throw std::system_error{
                errno, std::system_category(), "sq.ring MAP_FAILED"};

        if (p.features & IORING_FEAT_SINGLE_MMAP) {
            cq.ring_ptr = sq.ring_ptr;
        } else {
            cq.ring_ptr = mmap(
                nullptr, cq.ring_sz, PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_POPULATE, fd, IORING_OFF_CQ_RING);
            if (cq.ring_ptr == MAP_FAILED) [[unlikely]] {
                // don't forget to clean up sq
                cq.ring_ptr = nullptr;
                unmapRings();
                throw std::system_error{
                    errno, std::system_category(), "cq.ring MAP_FAILED"};
            }
        }

        sq.setOffset(p.sq_off);

        const size_t sqes_size = p.sq_entries * sizeof(io_uring_sqe);
        sq.sqes = reinterpret_cast<io_uring_sqe *>(mmap(
            0, sqes_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd,
            IORING_OFF_SQES));
        if (sq.sqes == MAP_FAILED) [[unlikely]] {
            unmapRings();
            throw std::system_error{
                errno, std::system_category(), "sq.sqes MAP_FAILED"};
        }

        cq.setOffset(p.cq_off);
    }

    inline void unmapRings() noexcept {
        munmap(sq.ring_ptr, sq.ring_sz);
        if (cq.ring_ptr && cq.ring_ptr != sq.ring_ptr)
            munmap(cq.ring_ptr, cq.ring_sz);
    }

    inline bool isSQRingNeedEnter(unsigned &flags) const noexcept {
        if (!(this->flags & IORING_SETUP_SQPOLL)) return true;

        if (IO_URING_READ_ONCE(*sq.kflags) & IORING_SQ_NEED_WAKEUP)
            [[unlikely]] {
            flags |= IORING_ENTER_SQ_WAKEUP;
            return true;
        }

        return false;
    }

    inline bool isCQRingNeedFlush() const noexcept {
        return IO_URING_READ_ONCE(*sq.kflags) & IORING_SQ_CQ_OVERFLOW;
    }
};

} // namespace liburingcxx
