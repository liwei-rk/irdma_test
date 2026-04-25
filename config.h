#ifndef CONFIG_H
#define CONFIG_H

#define HAVE_STATEMENT_EXPR 1
#define HAVE_BUILTIN_TYPES_COMPATIBLE_P 1
#define HAVE_TYPEOF 1
#define HAVE_ISBLANK 1
#define HAVE_BUILTIN_CLZ 1
#define HAVE_BUILTIN_CLZL 1

#define PACKAGE_VERSION "39.0"

#define RDMA_CDEV_DIR "/dev/infiniband"

#define IBV_CONFIG_DIR "/etc/rdma"
#define RS_CONF_DIR "/etc/rdma/rsocket"
#define IWPM_CONFIG_FILE "/etc/iwpmd.conf"

#define VERBS_PROVIDER_DIR "/usr/lib/libibverbs"
#define VERBS_PROVIDER_SUFFIX ".so"
#define IBVERBS_PABI_VERSION 1

#define HAVE_FUNC_ATTRIBUTE_ALWAYS_INLINE 1
#define HAVE_FULL_SYMBOL_VERSIONS 1

#define SIZEOF_LONG 8

#define VERBS_IOCTL_ONLY 0
#define VERBS_WRITE_ONLY 0

#endif