# liburingcxx

A C++ binding of axboe/liburing. Reordered submission is supported.
Higher performance is offered by C++ constexpr.

axboe/liburing 的 C++ 实现，与原版不同，支持了 sqe 的乱序申请和提交，以更灵活地支持多线程应用。
利用 C++ 编译期常量，取得比原版更高的性能。

## Supported syscalls

- `readv` `writev` `read` `write` `recvmsg` `sendmsg`
- `nop` `timeout` `timeout_remove` `timeout_update`
- `accept` `accept_direct` `connect` `close` `send`
- `recv` `shutdown` `cancel` `cancel_fd`
- `splice` `tee` `poll_add` `poll_remove` `fsync`
- `cancel` `link_timeout` `files_update` `fallocate`
- `openat` `openat_direct` `statx` `fadvise` `madvise`
- `openat2` `openat2_direct` `provide_buffers` `remove_buffers`
- `unlinkat` `renameat` `sync_file_range` `mkdirat`
- `symlinkat` `linkat` `nop`
  
## Comming soon

- `epoll_ctl` `(un)register_eventfd` `(un)register_files`
- `(un)register_ring_fd` `(un)register_iowq_aff`
- `(un)register_buffers` ...

## Long Term Todo

- [x] Make io_uring's `flags` constexpr;
  - [x] then optimize `if` to `if constexpr`.
- [ ] Reconsider whether to use exceptions