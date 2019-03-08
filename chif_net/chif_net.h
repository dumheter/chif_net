/*
  MIT License

  Copyright (c) 2018 Christoffer Gustafsson

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef CHIF_NET_H_
#define CHIF_NET_H_

/**
 * @date 2017-12-11
 * @authors Christoffer Gustafsson
 *
 * chif_net is a light cross-platform socket library.
 */

#if defined(__cplusplus)
extern "C"
{
#endif

// ====================================================================== //
// Headers & Constants
// ====================================================================== //

// Platform detection and platform headers
#if defined(_WIN32) || defined(_WIN64)
#define CHIF_NET_WINDOWS
#define CHIF_NET_WINSOCK2
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <winerror.h>
#include <ws2def.h>
#pragma comment(lib, "Ws2_32.lib")
#elif defined(__linux__) || defined(__APPLE__) || defined(__GNUC__)
#define CHIF_NET_BERKLEY_SOCKET
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/select.h> //fd_set and select
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// linux only
#if defined(__linux__)
#include <linux/icmp.h>
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * Only linux allows for some more advanced tcp setting.
 **/
#if defined(__linux__)
#define CHIF_NET_HAS_TCP_DETAILS
#else
#define MSG_NOSIGNAL 0
#endif

// Inline
//# define CHIF_NET_INLINE inline
#define CHIF_NET_INLINE

  // ====================================================================== //
  // Macros
  // ====================================================================== //

#if defined(CHIF_NET_WINSOCK2)
// Invalid socket value
#define CHIF_NET_INVALID_SOCKET ((chif_net_socket)INVALID_SOCKET)
// Socket error value
#define CHIF_NET_SOCKET_ERROR ((int)SOCKET_ERROR)
#elif defined(CHIF_NET_BERKLEY_SOCKET)
// Invalid socket value
#define CHIF_NET_INVALID_SOCKET ((chif_net_socket)-1)
// Socket error value
#define CHIF_NET_SOCKET_ERROR ((int)-1)
#endif

#define CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(var) (void)var

// default argument for listen
#define CHIF_NET_DEFAULT_BACKLOG 128

// Minimum string length for when translating an iv4 socket struct address to
// string address representation.
#define CHIF_NET_IPV4_STRING_LENGTH INET_ADDRSTRLEN
// Minimum string length for when translating an iv6 socket struct address to
// string address representation.
#define CHIF_NET_IPV6_STRING_LENGTH INET6_ADDRSTRLEN
// Can hold both ipv4 and ipv6 addresses represented as a string.
#define CHIF_NET_IPVX_STRING_LENGTH INET6_ADDRSTRLEN

// When binding to a port, use this to have the kernel select an unsued port.
#define CHIF_NET_UNUSED_PORT 0

  // ====================================================================== //
  // Types
  // ====================================================================== //

  // boolean used for setsockopt calls
  typedef int chif_net_opt_bool;

  typedef uint16_t chif_net_port;

#if defined(CHIF_NET_WINSOCK2)
  typedef SOCKET chif_net_socket;
  typedef long long ssize_t;
  typedef TIMEVAL timeval;
  typedef unsigned long nfds_t;
#elif defined(CHIF_NET_BERKLEY_SOCKET)
typedef int chif_net_socket;
#endif

  typedef enum
  {
    CHIF_NET_RESULT_UNKNOWN = 0,
    CHIF_NET_RESULT_SUCCESS,
    CHIF_NET_RESULT_LIBRARY_NOT_INITIALIZED,
    CHIF_NET_RESULT_BLOCKING,
    CHIF_NET_RESULT_MAX_SOCKETS_REACHED,
    CHIF_NET_RESULT_NOT_A_SOCKET,
    CHIF_NET_RESULT_WOULD_BLOCK,
    CHIF_NET_RESULT_CONNECTION_REFUSED,
    CHIF_NET_RESULT_INVALID_ADDRESS,
    CHIF_NET_RESULT_INVALID_FILE_DESCRIPTOR,
    CHIF_NET_RESULT_ACCESS_DENIED,
    CHIF_NET_RESULT_SOCKET_ALREADY_IN_USE,
    CHIF_NET_RESULT_NO_FREE_PORT,
    CHIF_NET_RESULT_IN_PROGRESS,
    CHIF_NET_RESULT_ALREADY_CONNECTED,
    CHIF_NET_RESULT_TIMEDOUT,
    CHIF_NET_RESULT_CONNECTION_ABORTED,
    CHIF_NET_RESULT_NOT_LISTENING_OR_NOT_CONNECTED,
    CHIF_NET_RESULT_NO_FREE_FILE_DESCRIPTORS,
    CHIF_NET_RESULT_NO_FREE_FILES,
    CHIF_NET_RESULT_SOCKET_RESET,
    CHIF_NET_RESULT_CONNECTION_CLOSED,
    CHIF_NET_RESULT_NOT_VALID_ADDRESS_FAMILY,
    CHIF_NET_RESULT_NOT_ENOUGH_SPACE,
    CHIF_NET_RESULT_NETWORK_SUBSYSTEM_FAILED,
    CHIF_NET_RESULT_INVALID_INPUT_PARAM,
    CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED,
    CHIF_NET_RESULT_TOO_LONG_MSG_NOT_SENT,
    CHIF_NET_RESULT_FAIL,
    CHIF_NET_RESULT_INVALID_PROTOCOL,
    CHIF_NET_RESULT_NO_MEMORY,
    CHIF_NET_RESULT_NO_NETWORK,
    CHIF_NET_RESULT_BLOCKING_CANCELED,
    CHIF_NET_RESULT_NET_UNREACHABLE,
    CHIF_NET_RESULT_BUFSIZE_INVALID,
    CHIF_NET_RESULT_NAME_SERVER_FAIL,
    CHIF_NET_RESLUT_NO_NAME
  } chif_net_result;

  typedef enum
  {
    CHIF_NET_PROTOCOL_TCP,
    CHIF_NET_PROTOCOL_UDP,
    CHIF_NET_PROTOCOL_ICMP,
    CHIF_NET_PROTOCOL_RAW
  } chif_net_protocol;

  typedef enum
  {
    CHIF_NET_ADDRESS_FAMILY_IPV4,
    CHIF_NET_ADDRESS_FAMILY_IPV6,
    CHIF_NET_ADDRESS_FAMILY_UNIX
  } chif_net_address_family;

  /**
   * Structure to store both ipv4 and ipv6 addresses.
   */
  typedef struct
  {
    struct sockaddr_storage addr;
  } chif_net_address;

  // ====================================================================== //
  // Definition
  // ====================================================================== //

  /**
   * Called to start up the chif network library.
   * Only required when using winsock library.
   * @return Result of the operation
   */
  chif_net_result chif_net_startup();

  /**
   * Called to shut down the chif network library.
   * Only required when using winsock library.
   * @return Result of the operation.
   */
  chif_net_result chif_net_shutdown();

  /**
   * Open a socket that uses the specified transport protocol for data
   * transmission.
   *
   * NOTE FOR LINUX USERS when using RAW SOCKETS:
   * Raw sockets on linux typically require elevated privileges to run.
   * A way to get around that is to give the binary capabilities, like so:
   * > sudo setcap cap_net_raw=ep [FILE]
   *
   * @param transport_protocol Transport protocol to use.
   * @param address_family IP address family.
   * @param socket The opened socket.
   * @return Result of the operation.
   */
  chif_net_result chif_net_open_socket(chif_net_socket* socket,
                                       chif_net_protocol transport_protocol,
                                       chif_net_address_family address_family);

  /**
   * Closes a socket that was previously opened with the open socket function.
   *
   * @param socket The socket to close.
   * @return
   */
  chif_net_result chif_net_close_socket(chif_net_socket* socket);

  /**
   * Connect to a IP address.
   * @param socket Open a socket before call with chif_net_open_socket
   * @param address Prepare the address with chif_net_create_address
   * @return Result of the operation.
   */
  chif_net_result chif_net_connect(chif_net_socket socket,
                                   chif_net_address* address);

  /**
   * Bind a socket to the port
   * @param socket
   * @param port
   * @param address_family
   * @return
   */
  chif_net_result chif_net_bind(chif_net_socket socket,
                                chif_net_port port,
                                chif_net_address_family address_family);

  /**
   * Start listening for connections on a socket.
   * @param socket
   * @param maximum_backlog Queue length for sockets waiting to be accepted.
   *                        Use CHIF_NET_DEFAULT_BACKLOG for default.
   * @return
   */
  chif_net_result chif_net_listen(chif_net_socket socket, int maximum_backlog);

  /**
   *
   * @param server_socket
   * @param client_address
   * @param client_socket
   * @return Result
   */
  chif_net_result chif_net_accept(chif_net_socket server_socket,
                                  chif_net_address* client_address,
                                  chif_net_socket* client_socket);

  /**
   * Read data from the socket. Will block if blocking is set and cannot read.
   * If supplied buffer is smaller than the data available, depending on the
   * protocol used, the excess data may be discarded. For SOCK_STREAM: Since
   * this is considered a stream, data will not be discarded and is available
   * on next call. For SOCK_DGRAM: The excess data is discarded.
   *
   * @param socket
   * @param buf
   * @param bufsize
   * @param read_bytes
   * @return Result of the operation.
   */
  chif_net_result chif_net_read(chif_net_socket socket,
                                uint8_t* buf,
                                size_t bufsize,
                                ssize_t* read_bytes);

  /**
   * Like chif_net_read, but places the source addr of the message in srcaddr.
   *
   * TODO test this function
   *
   * @param socket
   * @param buf
   * @param bufsize
   * @param read_bytes
   * @param srcaddr
   * @return Result of the operation.
   */
  chif_net_result chif_net_readfrom(chif_net_socket socket,
                                    uint8_t* buf,
                                    size_t bufsize,
                                    ssize_t* read_bytes,
                                    chif_net_address* srcaddr);

  /**
   * Write to a socket. Will block if buffer does not fit in the send buffer.
   * Unless nonblock io mode is set, then it will return error. You can use
   * chif_net_can_write to check if the interface is ready to write more data.
   * @param socket
   * @param buf
   * @param bufsize
   * @param sent_bytes
   * @return Result of the operation.
   */
  chif_net_result chif_net_write(chif_net_socket socket,
                                 const uint8_t* buf,
                                 size_t bufsize,
                                 ssize_t* sent_bytes);

  /**
   * Write to a socket, just as chif_net_write, but has a target address option.
   * @param socket
   * @param buf
   * @param bufsize
   * @param sent_bytes
   * @param target_addr
   * @return Result of the operation.
   */
  chif_net_result chif_net_writeto(chif_net_socket socket,
                                   const uint8_t* buf,
                                   size_t bufsize,
                                   ssize_t* sent_bytes,
                                   chif_net_address* target_addr);

  /**
   * Can we read without blocking? Is there anything to read?
   * If the socket is in listening state it will check if we can call accept
   * See chif_net_get_bytes_available to get amount of bytes that can be read.
   * @param socket
   * @param socket_can_read
   * @param timeout_ms How long should we wait before accepting a negative
   * response?
   * @return
   */
  chif_net_result chif_net_can_read(chif_net_socket socket,
                                    int* socket_can_read,
                                    int timeout_ms);

  /**
   * Can we write without blocking?
   * without blocking.
   * @param socket
   * @param socket_can_write
   * @param timeout_ms How long should we wait before accepting a negative
   * response?
   * @return
   */
  chif_net_result chif_net_can_write(chif_net_socket socket,
                                     int* socket_can_write,
                                     int timeout_ms);

  /**
   * Check if the socket has any errors.
   * @param socket
   * @return If no error, CHIF_NET_RESULT_SUCCESS will be returned.
   */
  chif_net_result chif_net_has_error(chif_net_socket socket);

  /**
   * Sets the blocking mode of a socket to either blocking or non-blocking
   * depending on the specified flag.
   * @param socket Socket to set blocking mode on.
   * @param blocking True to make operations on the socket blocking. False to
   * make operations on the socket non-blocking.
   * @return Result of the operation.
   */
  chif_net_result chif_net_set_socket_blocking(chif_net_socket socket,
                                               bool blocking);

  /**
   * Create an address structure with the exact information. Note, will
   * not perform DNS lookup for you. For that, use @chif_net_lookup_address.
   * @param address Output address
   * server_address{};
   * @param ip_address String representation of ip address "ddd.ddd.ddd.ddd".
   * Example, "127.0.0.1"
   * @param port
   * @param address_family Example, ipv4
   * @return
   */
  chif_net_result chif_net_create_address(
    chif_net_address* address_out,
    const char* ip_address,
    chif_net_port port,
    chif_net_address_family address_family);

  /**
   * Find address from information. Will automagically find the address
   * by doing DNS lookup, etc.
   * @param address_out Output address
   * @param name Example, "127.0.0.1" or "www.duckduckgo.com".
   * @param service Example, "http" or "80".
   * @param address_family
   * @param transport_protocol
   */
  chif_net_result chif_net_lookup_address(
      chif_net_address* address_out,
      const char* name,
      const char* service,
      chif_net_address_family address_family,
      chif_net_protocol transport_protocol);

  /*
   * Get the address of a socket.
   * @param socket
   * @param address
   */
  chif_net_result chif_net_get_address(chif_net_socket socket,
                                       chif_net_address* address);

  /**
   * Get the address of the peer that the socket is connected to.
   */
  chif_net_result chif_net_get_peer_address(chif_net_socket socket,
                                            chif_net_address* address);

  /*
   * From a socket, get the IP address of it as a string in the
   * dotted four octet format.
   * @param socket
   * @param str
   * @param strlen Should be at least CHIF_NET_IPV4_STRING_LENGTH for ipv4, and
   * CHIF_NET_IPV6_STRING_LEGNTH for ipv6. Use CHIF_NET_IPVX_STRING_LENGTH to
   * contain either.
   * @return
   */
  chif_net_result chif_net_ip_from_socket(chif_net_socket socket,
                                          char* str,
                                          size_t strlen);

  /**
   * From an address, get the IP address of it as a string in the
   * dotted four octet format.
   * @param socket
   * @param str Output parameter
   * @param strlen Should be at least CHIF_NET_IPV4_STRING_LENGTH for ipv4, and
   * CHIF_NET_IPV6_STRING_LEGNTH for ipv6. Use CHIF_NET_IPVX_STRING_LENGTH to
   * contain either.
   * @return
   */
  chif_net_result chif_net_ip_from_address(const chif_net_address* address,
                                           char* str,
                                           size_t strlen);

  /*
   * From a socket, get the port of it.
   * @param socket
   * @param port
   * @return
   */
  chif_net_result chif_net_port_from_socket(chif_net_socket socket,
                                            chif_net_port* port);

  /*
   * From an address, get the port of it.
   * @param address
   * @param port
   * @return
   */
  chif_net_result chif_net_port_from_address(const chif_net_address* address,
                                             chif_net_port* port);

  /**
   * Get number of bytes available for read on given socket.
   * @param socket
   * @param bytes_available Amount of bytes available to read.
   * @return
   */
  chif_net_result chif_net_get_bytes_available(chif_net_socket socket,
                                               unsigned long* bytes_available);

  /**
   * Allow reuse of addresses. Best used before calling bind.
   * If CHIF_RESULT_NOT_LISTENING_OR_NOT_CONNECTED error is returned, either
   * the socket is closed or the protocol does not support this option.
   * @param socket
   * @param reuse Boolean option
   * @return
   */
  chif_net_result chif_net_set_reuse_addr(chif_net_socket socket,
                                          chif_net_opt_bool reuse);

  /**
   * Not possible on windows platform.
   * @param socket
   * @param reuse To reuse or not, 0 for no, 1 for yes.
   * @return
   */
  chif_net_result chif_net_set_reuse_port(chif_net_socket socket,
                                          chif_net_opt_bool reuse);

  /**
   * Set the connection to keep it alive, if supported by the protocol. Useless
   * for connectionless protocols such as UDP.
   * @param socket
   * @param keepalive Boolean option
   * @return
   */
  chif_net_result chif_net_set_keepalive(chif_net_socket socket,
                                         chif_net_opt_bool keepalive);

  /**
   * Enables broadcast privileges, if supported by the protocol. Typically used
   * with the UDP protocol.
   * @param socket
   * @param broadcast Boolean option
   * @return
   */
  chif_net_result chif_net_set_broadcast(chif_net_socket socket,
                                         chif_net_opt_bool broadcast);

  /**
   * Set the timeout for blocking receive calls.
   * @param socket
   * @param time_ms
   * @return
   */
  chif_net_result chif_net_set_recv_timeout(chif_net_socket socket,
                                            int time_ms);

  /**
   * Set the timeout for blocking send calls.
   * @param socket
   * @param time_ms
   * @return
   */
  chif_net_result chif_net_set_send_timeout(chif_net_socket socket,
                                            int time_ms);

  /**
   * Specify the maximum amount of time in milliseconds that
   * transmitted data may remain unacknowledged before TCP will forcibly close
   * the connection.  If the option value is specified as 0, TCP will to use the
   * system default.
   *
   * This option, like many others, will be inherited by the socket returned by
   * accept(2), if it was set on the listening socket.
   *
   * Info:
   * https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=dca43c75e7e545694a9dd6288553f55c53e2a3a3
   * More info: http://man7.org/linux/man-pages/man7/tcp.7.html
   * @param socket
   * @param ms Time in milliseconds that we wait before timeout
   * @return
   */
  chif_net_result chif_net_tcp_set_user_timeout(chif_net_socket socket,
                                                int time_ms);

  /* chif_net_result */
  /* chif_net_tcp_get_user_timeout(chif_net_socket socket, int time_ms); */

  /**
   * If set, disable the Nagle algorithm. This means that segments are always
   * sent out as soon as possible, even if there is only a small amount of data.
   * @param socket
   * @param nodelay
   * @return
   */
  chif_net_result chif_net_tcp_set_nodelay(chif_net_socket socket,
                                           chif_net_opt_bool nodelay);

  /**
   * Set the number of SYN retransmits that TCP should send before aborting the
   * attempt to connect.  It cannot exceed 255. This option should not be used
   * in code intended to be portable.
   * @param socket
   * @param count Maximum 255
   * @return
   */
  chif_net_result chif_net_tcp_set_syncnt(chif_net_socket socket,
                                          chif_net_opt_bool count);

  /**
   * Set the time to live (ttl) parameter in the IP header. This value will
   * determine how many routers the packet can hop through.
   * @param sock
   * @param ttl
   * @return
   */
  chif_net_result chif_net_set_ttl(chif_net_socket sock, int ttl);

  /**
   * Do we want to provide our own ip header?
   * @param sock
   * @param provide_own_hdr 0 means no, 1 means yes, the user will build hdr.
   * @return
   */
  chif_net_result chif_net_set_own_iphdr(chif_net_socket sock,
                                         int provide_own_hdr);

  /**
   * Build a ICMP packet. Will not provide an IP header, make sure your OS
   * provides one, see function chif_net_set_own_hdr.
   * @param buf Where the icmp packet will be stored.
   * @param bufsize On success, will set packet size here.
   * @param data The data to be put in the icmp data field.
   * @param data_size
   * @param id ICMP id, will appear in the echo response.
   * @param seq ICMP sequence, will appear in the echo response.
   * @return If it succeeded
   */
  chif_net_result chif_net_icmp_build(uint8_t* buf,
                                      size_t* bufsize,
                                      const void* data,
                                      size_t data_size,
                                      uint16_t id,
                                      uint16_t seq);

  /**
   * Convert the enumerated result to a string, good for printing the result.
   * @param
   * @return Ptr to the string.
   */
  const char* chif_net_result_to_string(chif_net_result result);

#if defined(__cplusplus)
}
#endif

#endif // CHIF_NET_H
