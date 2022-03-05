# liburingcxx
A C++ binding of axboe/liburing

## Supported syscalls

- `readv` `writev`

## Long Term Todo

- Make io_uring's `flags` constexpr;
  - then optimize `if` to `if constexpr`.
- Reconsider whether to use exceptions