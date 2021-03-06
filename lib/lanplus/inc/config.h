/* config.h.  Generated by configure.  */
/* config.h.in.  Generated from configure.in by autoheader.  */
/* Define to 1 to enable all command line options. */
#define ENABLE_ALL_OPTIONS 1
/* Define to 1 for extra file security. */
/* #undef ENABLE_FILE_SECURITY */
/* Define to 1 if you have the `alarm' function. */
#define HAVE_ALARM 1
#ifdef WIN32
#undef HAVE_ARPA_INET_H
#undef HAVE_BYTESWAP_H
#else
/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1
/* Define to 1 if you have the <byteswap.h> header file. */
#define HAVE_BYTESWAP_H 1
#endif
/* Define to 1 if libcrypto supports MD2. */
#define HAVE_CRYPTO_MD2 1
/* Define to 1 if libcrypto supports MD5. */
#define HAVE_CRYPTO_MD5 1
#ifdef WIN32
#undef HAVE_DLFCN_H
#undef HAVE_FCNTL_H
#undef HAVE_INTTYPES_H
#undef HAVE_NETDB_H
#undef HAVE_NETINET_IN_H
#undef HAVE_SYS_IOCTL_H
#undef HAVE_OPENIPMI_H
#else
/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1
/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1
/* Define to 1 if you have the <sys/ipmi.h> header file. */
/* #undef HAVE_FREEBSD_IPMI_H */
/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1
/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1
/* Define to 1 if you have the <linux/ipmi.h> header file. */
#define HAVE_OPENIPMI_H 1
/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1
#endif
/* Define to 1 if you have the `gethostbyname' function. */
#define HAVE_GETHOSTBYNAME 1
/* Define to 1 if you have the `getpassphrase' function. */
/* #undef HAVE_GETPASSPHRASE */
/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1
/* Define to 1 if your system has a GNU libc compatible `malloc' function, and
   to 0 otherwise. */
#define HAVE_MALLOC 1
/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1
/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1
/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1
/* Define to 1 if you have the <paths.h> header file. */
#define HAVE_PATHS_H 1
/* Define to 1 if readline present. */
#define HAVE_READLINE 1
/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1
/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET 1
/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1
/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1
/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1
/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1
/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1
/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1
/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1
/* Define to 1 if you have the <sys/byteorder.h> header file. */
/* #undef HAVE_SYS_BYTEORDER_H */
/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1
/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1
/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1
/* Define to 1 if you have <sys/termios.h>. */
/* #undef HAVE_SYS_TERMIOS_H */
/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1
/* Define to 1 if you have <termios.h>. */
#define HAVE_TERMIOS_H 1
/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1
/* Define to 1 to enable Solaris 10 BMC interface. */
/* #undef IPMI_INTF_BMC */
/* Define to 1 to enable Intel IMB interface. */
/* #undef IPMI_INTF_IMB, or #define IPMI_INTF_IMB 1 */
/* Define to 1 to enable LAN IPMIv1.5 interface. */
// #define IPMI_INTF_LAN 1
/* Define to 1 to enable LAN+ IPMIv2 interface. */
#define IPMI_INTF_LANPLUS 1
/* Define to 1 to enable Solaris 9 LIPMI interface. */
/* #undef IPMI_INTF_LIPMI */
/* Define to 1 to enable Linux OpenIPMI interface. */
// #define IPMI_INTF_OPEN 1
/* Name of package */
#define PACKAGE "ipmitool"
/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""
/* Define to the full name of this package. */
#define PACKAGE_NAME ""
/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""
/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""
/* Define to the version of this package. */
#define PACKAGE_VERSION ""
/* Define to the type of arg 1 for `select'. */
#define SELECT_TYPE_ARG1 int
/* Define to the type of args 2, 3 and 4 for `select'. */
#define SELECT_TYPE_ARG234 (fd_set *)
/* Define to the type of arg 5 for `select'. */
#define SELECT_TYPE_ARG5 (struct timeval *)
/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1
/* Version number of package */
#define VERSION "1.8.7"
/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */
/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */
/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif
/* Define to rpl_malloc if the replacement function should be used. */
/* #undef malloc */
