# Hướng dẫn Build và Sử dụng

## Yêu cầu hệ thống

- C++17 compiler (g++ 7+ hoặc clang++ 7+)
- pthread library
- Linux (epoll) hoặc BSD/macOS (kqueue)
- Make

## Build

### Linux

```bash
cd cpp
make
```

### Với OpenSSL (khuyến nghị cho DES)

```bash
# Cài đặt OpenSSL development libraries
sudo apt-get install libssl-dev  # Ubuntu/Debian
sudo yum install openssl-devel    # CentOS/RHEL

# Uncomment trong Makefile:
# CXXFLAGS += -DUSE_OPENSSL
# LDFLAGS += -lssl -lcrypto

make
```

### macOS

```bash
cd cpp
make
```

## Chạy

```bash
./bin/nvnc
```

## Tối ưu hóa cho 20k-100k threads

### 1. Tăng file descriptor limit

```bash
# Linux
ulimit -n 1000000
# Hoặc thêm vào /etc/security/limits.conf:
# * soft nofile 1000000
# * hard nofile 1000000
```

### 2. Tăng thread limit

```bash
# Linux
ulimit -u 100000
```

### 3. Tối ưu kernel parameters (Linux)

```bash
# /etc/sysctl.conf
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.ipv4.ip_local_port_range = 1024 65535
```

### 4. Compile với optimization

Makefile đã có `-O3 -march=native` để tối ưu.

## Lưu ý

- DES implementation hiện tại là placeholder. Để sử dụng đầy đủ, cần:
  - Implement full DES algorithm, hoặc
  - Sử dụng OpenSSL (uncomment trong Makefile)
  
- Thread pool và I/O multiplexing đã được tối ưu cho high concurrency
- Monitor memory và CPU khi chạy với nhiều threads

## Troubleshooting

### Lỗi compile: cannot find -lssl

Cài đặt OpenSSL development libraries hoặc comment out OpenSSL flags trong Makefile.

### Lỗi: Too many open files

Tăng file descriptor limit (xem trên).

### Performance không tốt

- Kiểm tra CPU và memory usage
- Giảm số threads nếu cần
- Sử dụng I/O multiplexing thay vì blocking sockets

