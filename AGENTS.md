# Build & Run

**Headers installed**: Added `pingpong.h`, `pingpong.c`, `config.h`, and `ccan/minmax.h`, `ccan/build_assert.h` from rdma-core source.

**Compile**: `make`

**Runtime requirements**:
- RDMA hardware (InfiniBand/iWARP/RoCE device)
- Huge pages: `/mnt/huge/` must be mounted and configured
- Run as server: `./irdma_test` (waits for connection on port 18515)
- Run as client: `./irdma_test <server-hostname>`

# Usage

## Basic Usage
```
Server side: ./irdma_test
Client side: ./irdma_test <server-hostname>
```

## Command Line Options
| Option | Description | Default |
|--------|-------------|---------|
| `-p, --port` | TCP port for connection exchange | 18515 |
| `-d, --ib-dev` | RDMA device name | first available |
| `-i, --ib-port` | RDMA device port | 1 |
| `-s, --size` | Buffer size per operation | 4096 |
| `-m, --mtu` | Path MTU (256/512/1024/2048/4096) | 1024 |
| `-n, --iters` | Number of test iterations | 1000 |
| `-w, --num_wqe` | Work queue entries per doorbell (max 32) | 1 |
| `-r, --rx-depth` | Receive queue depth | 127 |
| `-l, --sl` | Service level | 0 |
| `-g, --gid-idx` | GID index (for RoCE) | -1 |
| `-b, --dest_ip` | Destination IP for hardware | none |
| `-c, --chk` | Validate buffer data after transfer | disabled |
| `-a, --physical` | Use physical address instead of virtual | disabled |
| `-e, --events` | Use event-based CQ polling | disabled (poll mode) |

## Example Commands
```bash
# Basic test with 1MB buffer, 10 iterations
Server: ./irdma_test -s 1048576 -n 10
Client: ./irdma_test -s 1048576 -n 10 server-host

# RoCE test with GID index 3
Server: ./irdma_test -g 3
Client: ./irdma_test -g 3 server-host

# Performance test with multiple WQE (doorbell optimization)
Server: ./irdma_test -w 16 -s 65536 -n 100
Client: ./irdma_test -w 16 -s 65536 -n 100 server-host

# Data validation test
Server: ./irdma_test -c
Client: ./irdma_test -c server-host
```

## Output
The program reports:
- RDMA Write speed (Mbit/sec and latency)
- RDMA Read speed (Mbit/sec and latency)
- Cumulative statistics across iterations

# Architecture

## Design Overview
This is an RDMA performance benchmark tool implementing RDMA Write/Read operations:
1. **Connection Establishment**: TCP socket exchanges RDMA queue pair (QP) parameters
2. **Memory Registration**: Uses huge pages (1GB) for zero-copy DMA buffers
3. **RDMA Operations**: Client writes data → Client reads back (pingpong pattern)
4. **Completion Handling**: Polling mode by default (event mode optional)

## Key Components
- `struct irdma_context`: Main RDMA context holding device, PD, CQ, QP, MR references
- `start_buf`: Source buffer for RDMA Write operations
- `rdma_buf`: Destination buffer for RDMA Read operations
- `rdma_buf_info`: Exchanged buffer metadata (virtual/physical address, rkey, size)

## Memory Model
- **Huge Pages**: `/mnt/huge/start_buf-port%d` and `/mnt/huge/rdma_buf-port%d` (1GB each)
- **Physical Address**: Optional via `/proc/self/pagemap` for hardware requiring physical addresses
- **Fallback**: Uses anonymous mmap if huge pages unavailable (`-o` disables huge pages)

## RDMA Workflow (Client-side)
```
fill_start_buf()          → Populate source buffer with test data
irdma_setup_wr_start_buf()→ Configure RDMA Write work requests
ibv_post_send()           → Post RDMA Write to remote buffer
poll CQ                   → Wait for RDMA Write completion
irdma_setup_wr_rdma_buf() → Configure RDMA Read work requests
ibv_post_send()           → Post RDMA Read from remote buffer
poll CQ                   → Wait for RDMA Read completion
validate_buf()            → Optional: compare start_buf vs rdma_buf
```

## Performance Features
- **Doorbell optimization**: Multiple WQE per doorbell (`-w` option)
- **Zero-copy**: Direct DMA transfer via registered MRs
- **Minimal CPU**: RDMA bypasses kernel, remote side has no CPU involvement

# Setup Requirements

## Huge Pages Configuration
```bash
# Allocate huge pages
echo 1024 > /proc/sys/vm/nr_hugepages

# Mount hugetlbfs
mkdir -p /mnt/huge
mount -t hugetlbfs nodev /mnt/huge

# Verify
cat /proc/meminfo | grep HugePages
```

## RDMA Device Check
```bash
# List RDMA devices
ibv_devices

# Check device status
ibv_devinfo
```