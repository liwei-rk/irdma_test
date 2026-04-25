# Troubleshooting

## Quick Reference

| Error | Likely Cause | Quick Fix |
|-------|-------------|-----------|
| `errno=110 (ETIMEDOUT)` | Cross-subnet hop_limit or firewall | Use updated code + open UDP 4791 |
| `errno=22 (EINVAL)` | Invalid GID index or MTU | Check `ibv_show_gid` + `-g` option |
| `Failed to get quit message` | Client crashed earlier | Check client-side errors first |
| `No IB devices found` | Hardware/driver issue | `modprobe rdma` + check `/dev/infiniband/` |

## Connection Errors

### errno=110 (Connection timed out)

**Cross-subnet RoCEv2 (Most Common)**

Symptoms: Ping works, but RDMA connection fails across different subnets.

Root cause: `hop_limit=1` (single hop) cannot route across subnets.

**Fix**: Use updated code (hop_limit=64) + configure firewall:

```bash
# Open RoCEv2 UDP port (required!)
iptables -I INPUT -p udp --dport 4791 -j ACCEPT
iptables -I OUTPUT -p udp --dport 4791 -j ACCEPT
```

**RoCEv2 supports cross-subnet routing** (UDP/IP based, port 4791).

**GID Index (Required for RoCE)**

```bash
# Find GID containing your IP
ibv_show_gid <device> <port>

# Look for: ::ffff:192.168.x.x (IPv4-mapped GID)
Server: ./irdma_test -g <index>
Client: ./irdma_test -g <index> server-host
```

### errno=22 (Invalid parameter)

Check GID index matches network:
```bash
ibv_query_gid <device> <port> <index>
# GID should contain your IP address
```

Check MTU:
```bash
ibv_devinfo | grep mtu
./irdma_test -m <mtu>  # 256/512/1024/2048/4096
```

### "Failed to get quit message"

Client crashed/exited early. Check client logs for real error.

## Device Errors

### "No IB devices found"

```bash
# Load driver
modprobe rdma

# Check permissions
ls -l /dev/infiniband/

# Verify device
ibv_devices
ibv_devinfo
```

### "Couldn't get local LID" (InfiniBand only)

```bash
# Start subnet manager
opensm
```

### "Huge page allocation failed"

Auto-fallback to normal memory (performance may degrade).

Force normal memory:
```bash
./irdma_test -o
```

Configure huge pages (better performance):
```bash
echo 1024 > /proc/sys/vm/nr_hugepages
mkdir -p /mnt/huge
mount -t hugetlbfs nodev /mnt/huge
```

## Network Types

| Type | Requirements | Command |
|------|-------------|---------|
| InfiniBand | opensm running | `./irdma_test` (default) |
| RoCEv2 | GID index + UDP 4791 firewall | `./irdma_test -g <index>` |
| iWARP | Check device caps | `./irdma_test -m 1024` |

**RoCEv1 vs RoCEv2**:
- RoCEv1: L2 only, cannot cross subnet
- RoCEv2: UDP/IP (port 4791), **can cross subnet**

## Performance

**Slow transfer**:
```bash
# Larger buffer
./irdma_test -s 1048576

# Doorbell optimization
./irdma_test -w 16 -s 65536

# Use huge pages (default, check startup message)
```

**High latency**: Check MTU match, QoS (`-l`), network congestion.

## Debug Tools

```bash
# GID info
ibv_show_gid <dev> <port>
ibv_query_gid <dev> <port> <idx>

# Device status
ibv_devinfo

# Connectivity
ibtracert <dest>
ibqueryerrors

# Basic RDMA ping
rping -s -a <ip> -v   # server
rping -c -a <ip> -v   # client
```

## Firewall Configuration

**RoCEv2 requires UDP port 4791**:

```bash
# iptables
iptables -I INPUT -p udp --dport 4791 -j ACCEPT
iptables -I OUTPUT -p udp --dport 4791 -j ACCEPT

# ufw
ufw allow 4791/udp

# firewalld
firewall-cmd --add-port=4791/udp --permanent
firewall-cmd --reload
```

**InfiniBand**: No firewall configuration needed (L2 network).