# liburingcxx

A C++ implement of axboe/liburing. Reordered submissions are supported.
Higher performance is achieved by C++ constexpr.

axboe/liburing 的 C++ 实现，与原版不同，支持了 sqe 的乱序申请和提交，以更灵活地支持多线程应用。
利用 C++ 编译期常量，取得比原版更高的性能。

**提示**：liburingcxx 的最新版本发布于[co_context](https://github.com/Codesire-Deng/co_context)，本仓库**跟从更新**。

## Supported syscalls

- `read{,v,v2,_fixed}` `write{,v,v2,_fixed}`
- `recvmsg{,_multishot}` `sendmsg`
- `nop` `timeout` `timeout_remove` `timeout_update`
- `accept{,_direct}` `connect` `close` `send`
- `recv{,_multishot}` `shutdown` `cancel{,_fd}`
- `splice` `tee` `poll_add` `poll_remove` `fsync`
- `cancel` `link_timeout` `files_update` `fallocate`
- `openat{,2,_direct,2_direct}` `statx` `fadvise` `madvise`
- `openat2_direct` `provide_buffers` `remove_buffers`
- `unlinkat` `renameat` `sync_file_range` `mkdir{,at}`
- `symlink{,at}` `link{,at}` `nop` `msg_ring`
- `(un)register_buffers`

总计 57 个功能

## Comming soon

- `epoll_ctl` `(un)register_eventfd` `(un)register_files`
- `(un)register_iowq_aff`
- `(un)register_buffers` ...
