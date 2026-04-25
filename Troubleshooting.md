# Troubleshooting

## Common Errors

### "Failed to modify QP to RTR"

**Symptoms**:
- Client: `Failed to modify QP to RTR: errno=X (error message)`
- Server: `Failed to get quit message` (blocked waiting for client)

**Root Cause**: QP state transition failed, client exits early without sending "quit" message.

**Diagnosis Steps**:

1. Check RDMA device type:
```bash
ibv_devinfo | grep link_layer
```
- `InfiniBand` → Requires LID (auto-obtained)
- `Ethernet` → RoCE, requires GID index

2. For RoCE, specify GID index:
```bash
# Check available GID indexes
ibv_show_gid <device> <port>

# Test with GID index
Server: ./irdma_test -g <gid-index>
Client: ./irdma_test -g <gid-index> server-host
```

3. Check MTU configuration:
```bash
ibv_devinfo | grep mtu
```
Test with matching MTU:
```bash
./irdma_test -m <mtu>  # Options: 256, 512, 1024, 2048, 4096
```

**Common errno values**:
- `EINVAL (22)` → Invalid parameter (check GID index, MTU)
- `EOPNOTSUPP (95)` → Operation not supported by device
- `ENOMEM (12)` → Memory allocation failure
- `ETIMEDOUT (110)` → Connection timeout (see below)

### errno=110 (ETIMEDOUT) - Connection timed out

**Symptoms**: 
- Client: `Failed to modify QP to RTR: errno=110 (Connection timed out)`
- But ping works fine between servers

**Root Cause**: RDMA connection layer timeout, TCP layer is fine but RDMA QP cannot establish connection.

**Why ping works but RDMA fails**:
- Ping tests IP layer connectivity
- RDMA requires additional parameters: GID, LID, MTU, QP configuration

**Critical: Check BOTH sides' errors**:
The program flow is:
1. Server accepts TCP connection → reads client QP params
2. **Server tries to connect to client QP first** (check if server also reports error!)
3. Server sends its QP params to client
4. Client tries to connect to server QP

**Diagnosis steps**:

1. Check if server also reports error:
```bash
# Look for: "Server: Couldn't connect to client QP"
# If server fails, client will also fail
```

2. For RoCE networks - MUST specify GID index:
```bash
# List GID indexes
ibv_show_gid <device> <port>

# Check which GID corresponds to your IP
# GID contains IPv4/IPv6 address in lower 64 bits

# Example output:
# Index 0: fe80::... (link-local)
# Index 1: ::ffff:192.168.1.100 (IPv4 mapped)
# Index 2: 2001:... (IPv6)

# Use IPv4-mapped GID for IPv4 networks
Server: ./irdma_test -g <gid-index>
Client: ./irdma_test -g <gid-index> server-host
```

3. Verify GID matches your network:
```bash
# Check GID index content
ibv_query_gid <device> <port> <index>

# Example: Check if GID matches server IP
# If server IP is 192.168.1.100
# GID should be: ::ffff:192.168.1.100
```

4. Check MTU compatibility:
```bash
ibv_devinfo | grep mtu
# Client and server must use same MTU
./irdma_test -m <mtu-value>
```

5. Check RDMA network configuration:
```bash
# For RoCEv2, check if switches support DSCP/ECN
# For InfiniBand, check if subnet manager running

# Verify RDMA traffic can flow
ibtracert <server-ip>
```

**Most common fix**:
```bash
# RoCE networks: Always specify GID index
Server: ./irdma_test -g 1  # or appropriate GID index
Client: ./irdma_test -g 1 server-host

# InfiniBand: Ensure subnet manager running
opensm
```

### "Failed to get quit message"

**Symptoms**: Server blocks waiting for "quit" message from client.

**Root Cause**: Client crashed or exited early (e.g., QP modification failure).

**Solution**: Check client-side errors first. This error is typically a symptom of earlier client failure.

### "No IB devices found"

**Symptoms**: `No IB devices found` error at startup.

**Diagnosis**:
```bash
ibv_devices
ibv_devinfo
```

**Possible causes**:
- RDMA hardware not installed
- RDMA driver not loaded
- Device permissions issue

**Solution**:
```bash
# Load RDMA modules
modprobe rdma

# Check device permissions
ls -l /dev/infiniband/
```

### "Huge page allocation failed"

**Symptoms**: `Huge page allocation failed, falling back to normal memory`

**Root Cause**: Huge pages not configured or insufficient.

**Solution**: Either:
1. Configure huge pages (recommended for performance):
```bash
echo 1024 > /proc/sys/vm/nr_hugepages
mkdir -p /mnt/huge
mount -t hugetlbfs nodev /mnt/huge
```

2. Or use `-o` option to disable huge pages:
```bash
./irdma_test -o
```

### "Couldn't get local LID"

**Symptoms**: `Couldn't get local LID` error on InfiniBand network.

**Root Cause**: InfiniBand subnet manager not running or SM not assigned LID.

**Solution**:
```bash
# Check subnet manager
opensm --version

# Start subnet manager (if not running)
opensm
```

## Network Type Configuration

### InfiniBand

Default configuration should work:
```bash
Server: ./irdma_test
Client: ./irdma_test server-host
```

Requirements:
- Subnet manager running (opensm)
- LID automatically assigned

### RoCE (RDMA over Converged Ethernet)

Must specify GID index:
```bash
# Find correct GID index
ibv_show_gid <device> <port>

# Example: GID index 3
Server: ./irdma_test -g 3
Client: ./irdma_test -g 3 server-host
```

Requirements:
- RoCE-capable NIC
- Proper network configuration
- Correct GID index for IPv4/IPv6

### iWARP

May require specific configuration:
```bash
# Check device capabilities
ibv_devinfo

# Use appropriate MTU
./irdma_test -m 1024
```

## Debug Mode

Enable debug output (requires modifying code):
```bash
# In irdma_test.c, line 52:
static int debug = 1;  # Change from 0 to 1
```

Recompile:
```bash
make clean && make
```

## Performance Issues

### Slow transfer speeds

**Possible causes**:
- Using normal memory instead of huge pages (check startup message)
- Small buffer size (try larger `-s` value)
- Single WQE per doorbell (try `-w` for doorbell optimization)

**Solutions**:
```bash
# Use huge pages
./irdma_test -s 1048576  # 1MB buffer

# Doorbell optimization
./irdma_test -w 16 -s 65536
```

### High latency

**Check**:
- MTU setting matches network capability
- Service level (`-l`) appropriate for QoS
- Network congestion

## Additional Debug Tools

```bash
# Monitor RDMA traffic
ibtracert <destination>

# Check RDMA statistics
ibqueryerrors

# Verify connectivity
rping -s -a <server-ip> -v
rping -c -a <server-ip> -v
```