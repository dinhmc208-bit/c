# VNC Scanner - C++ Implementation

## Tổng quan

Đây là phiên bản C++ của VNC Scanner, được tối ưu hóa để hỗ trợ 20k-100k luồng đồng thời.

## Tính năng

- ✅ Hỗ trợ IPv4 và IPv6
- ✅ Thread pool để quản lý hiệu quả nhiều luồng
- ✅ I/O multiplexing với epoll (Linux) / kqueue (BSD/macOS)
- ✅ Non-blocking sockets
- ✅ DES encryption cho VNC authentication
- ✅ RFB protocol handler
- ✅ CLI interface tương tự Python version

## Yêu cầu

- C++17 compiler (g++ hoặc clang++)
- pthread library
- Linux (epoll) hoặc BSD/macOS (kqueue)

## Build

```bash
cd cpp
make
```

## Sử dụng

```bash
./bin/nvnc
```

## Cấu trúc Project

```
cpp/
├── include/          # Header files
│   ├── config.h
│   ├── des.h
│   ├── net_tools.h
│   ├── rfb.h
│   ├── thread_pool.h
│   ├── io_multiplexer.h
│   ├── scan_engine.h
│   ├── brute_engine.h
│   ├── files.h
│   └── cli.h
├── src/              # Source files
│   ├── config.cpp
│   ├── des.cpp
│   ├── net_tools.cpp
│   ├── rfb.cpp
│   ├── thread_pool.cpp
│   ├── io_multiplexer.cpp
│   ├── scan_engine.cpp
│   ├── brute_engine.cpp
│   ├── files.cpp
│   ├── cli.cpp
│   └── main.cpp
├── Makefile
└── README_CPP.md
```

## Tối ưu hóa

### Thread Pool
- Sử dụng thread pool thay vì tạo thread cho mỗi task
- Giảm overhead của thread creation/destruction
- Hỗ trợ hàng chục nghìn tasks đồng thời

### I/O Multiplexing
- Epoll (Linux) hoặc kqueue (BSD/macOS)
- Non-blocking sockets
- Xử lý hàng trăm nghìn kết nối đồng thời

### Memory Management
- Sử dụng smart pointers
- Tránh memory leaks
- Hiệu quả về bộ nhớ

## So sánh với Python version

| Tính năng | Python | C++ |
|-----------|--------|-----|
| Max threads | ~1000-2000 | 20k-100k+ |
| Memory usage | Cao | Thấp |
| CPU usage | Trung bình | Tối ưu |
| Startup time | Chậm | Nhanh |
| Performance | Tốt | Rất tốt |

## Lưu ý

- Cần compile với optimization flags (-O3)
- Tăng ulimit cho file descriptors nếu cần
- Monitor memory và CPU usage khi chạy với nhiều threads

