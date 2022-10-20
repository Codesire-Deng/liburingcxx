# liburingcxx

A C++ implement of axboe/liburing. Reordered submissions are supported.
Higher performance is achieved by C++ constexpr.

axboe/liburing 的 C++ 实现，与原版不同，支持了 sqe 的乱序申请和提交，以更灵活地支持多线程应用。
利用 C++ 编译期常量，取得比原版更高的性能。

>  **提示**：liburingcxx 的最新版本发布于[co_context](https://github.com/Codesire-Deng/co_context)，本仓库**跟从更新**。

## Supported syscalls

As of 2022/10/20, **all** syscalls provided by liburing are supported. 

74 syscalls in total.
