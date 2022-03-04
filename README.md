# liburingcxx
A C++ binding of axboe/liburing

## Long Term Todo

- Make io_uring's `flags` constexpr;
  - then optimize `if` to `if constexpr`.