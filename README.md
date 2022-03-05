# liburingcxx
A C++ binding of axboe/liburing

## Supported syscalls

- `readv` `writev` `read` `write` `recvmsg` `sendmsg`
- `nop` `timeout` `timeout_remove` `timeout_update`
- `accept` `accept_direct` `connect` `close` `send`
- `recv` `shutdown`

## Comming soon...

- `splice` `tee` `poll_add` `poll_remove` `fsync`
- `cancel` `link_timeout` `files_update` `fallocate`
- `openat` `openat_direct` `statx` `fadvise` `madvise`
- `openat2` `openat2_direct` `epoll_ctl` `provide_buffers`
- `unlinkat` `renameat` `sync_file_range` `mkdirat`
- `symlinkat` `linkat`


## Long Term Todo

- Make io_uring's `flags` constexpr;
  - then optimize `if` to `if constexpr`.
- Reconsider whether to use exceptions