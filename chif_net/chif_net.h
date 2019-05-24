/**
 * MIT License
 *
 * Copyright (c) 2019 Christoffer Gustafsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef CHIF_NET_H_
#define CHIF_NET_H_

/**
 * @date 2017-12-11
 * @authors Christoffer Gustafsson
 *
 * chif_net is a light cross-platform socket library, aiming to provide
 * a unified API on Windows, Mac and Linux for the commonly used socket
 * functionality.
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
#elif defined(__linux__) || defined(__APPLE__) || defined(__GNUC__)
#define CHIF_NET_BERKLEY_SOCKET
#endif

#include <stddef.h>
#include <stdint.h>

// ====================================================================== //
// Macros
// ====================================================================== //

/**
 * Only linux allows for some more advanced tcp setting.
 **/
#if defined(__linux__)
#define CHIF_NET_HAS_TCP_DETAILS
#else
#define MSG_NOSIGNAL 0
#endif

#if defined(CHIF_NET_WINSOCK2)
#define CHIF_NET_INVALID_SOCKET ((chif_net_socket)(~0))
#elif defined(CHIF_NET_BERKLEY_SOCKET)
#define CHIF_NET_INVALID_SOCKET ((chif_net_socket)-1)
#endif

#define CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(var) (void)var

// Default argument for listen. How many connections can be pending.
#define CHIF_NET_DEFAULT_BACKLOG 128

// Minimum string length for when translating an iv4 socket struct address to
// string address representation.
#define CHIF_NET_IPV4_STRING_LENGTH 16 /*INET_ADDRSTRLEN*/
// Minimum string length for when translating an iv6 socket struct address to
// string address representation.
#define CHIF_NET_IPV6_STRING_LENGTH 46 /*INET6_ADDRSTRLEN*/
// Can hold both ipv4 and ipv6 addresses represented as a string.
#define CHIF_NET_IPVX_STRING_LENGTH CHIF_NET_IPV6_STRING_LENGTH

// Use this to let the OS decide the port.
#define CHIF_NET_ANY_PORT 0

// Use this to let the OS decide the address.
#define CHIF_NET_ANY_ADDRESS NULL

#define CHIF_NET_STATIC_ASSERT(condition, name)                                \
  typedef char name[(condition) ? 1 : -1]

// ====================================================================== //
// Types
// ====================================================================== //

// boolean used for setsockopt calls
typedef int chif_net_bool;
#define CHIF_NET_FALSE ((int)0)
#define CHIF_NET_TRUE ((int)1)

typedef uint16_t chif_net_port;

#if defined(CHIF_NET_WINSOCK2)
typedef uint64_t chif_net_socket;
#elif defined(CHIF_NET_BERKLEY_SOCKET)
typedef int chif_net_socket;
#endif

// TODO convert them back to errno in order to use strerrno()?
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
  CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY,
  CHIF_NET_RESULT_NOT_ENOUGH_SPACE,
  CHIF_NET_RESULT_NETWORK_SUBSYSTEM_FAILED,
  CHIF_NET_RESULT_INVALID_INPUT_PARAM,
  CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED,
  CHIF_NET_RESULT_TOO_LONG_MSG_NOT_SENT,
  CHIF_NET_RESULT_FAIL,
  CHIF_NET_RESULT_INVALID_TRANSPORT_PROTOCOL,
  CHIF_NET_RESULT_NO_MEMORY,
  CHIF_NET_RESULT_NO_NETWORK,
  CHIF_NET_RESULT_BLOCKING_CANCELED,
  CHIF_NET_RESULT_NET_UNREACHABLE,
  CHIF_NET_RESULT_BUFSIZE_INVALID,
  CHIF_NET_RESULT_NAME_SERVER_FAIL,
  CHIF_NET_RESLUT_NO_NAME,
  CHIF_NET_RESULT_BUFFER_BAD,
  CHIF_NET_RESULT_INVALID_SOCKTYPE
} chif_net_result;

typedef enum
{
  CHIF_NET_TRANSPORT_PROTOCOL_TCP = 6 /*IPPROTO_TCP*/,
  CHIF_NET_TRANSPORT_PROTOCOL_UDP = 17 /*IPPROTO_UDP*/
} chif_net_transport_protocol;

typedef enum
{
  CHIF_NET_ADDRESS_FAMILY_IPV4 = 2 /*AF_INET*/,
#if defined(CHIF_NET_WINSOCK2)
  CHIF_NET_ADDRESS_FAMILY_IPV6 = 23 /*AF_INET6*/
#else
  CHIF_NET_ADDRESS_FAMILY_IPV6 = 10 /*AF_INET6*/
#endif
} chif_net_address_family;

typedef struct
{
  uint16_t address_family;
  chif_net_port port;
  uint32_t address;
} chif_net_ipv4_address;

typedef struct
{
  uint16_t address_family;
  chif_net_port port;
  uint32_t flowinfo;
  uint32_t address[4];
  uint32_t scope_id;
} chif_net_ipv6_address;

/**
 * For best performance, explicitly use chif_netipv4_address when you can,
 * and cast it to chif_net_address for the function calls.
 */
typedef struct
{
  uint16_t address_family;
  uint8_t data[sizeof(chif_net_ipv6_address) - sizeof(uint16_t)];
} chif_net_address;

/**
 * @param socket The socket to check the events for.
 * @param request_events Fill out by bitmasking with chif_net_check_event
 * values.
 * @param events The returned events, check by bitmasking with
 * chif_net_check_event.
 */
typedef struct
{
  chif_net_socket socket;
  short request_events;
  short return_events;
} chif_net_check;

/**
 * Use in conjunction with chif_net_check.
 *
 * @param CHIF_NET_CHECK_EVENT_READ Can the socket read without blocking?
 * @param CHIF_NET_CHECK_EVENT_WRITE Can the socket write without blocking?
 * (Given that we don't write more than the socket can handle.)
 * @param CHIF_NET_CHECK_EVENT_ERROR Does the socket have any error?
 * @param CHIF_NET_CHECK_EVENT_CLOSED Is the socket in a closed state?
 * This only makes sense when using a connection based transport protocol.
 * NOTE: Ignored in request, will always be checked for.
 * @param CHIF_NET_CHECK_EVENT_INVALID Is the socket invalid?
 * NOTE: Ignored in request, will always be checked for.
 */
typedef enum
{
#if defined(CHIF_NET_WINSOCK2)
  CHIF_NET_CHECK_EVENT_READ = 0x0100 | 0x0200,
  CHIF_NET_CHECK_EVENT_WRITE = 0x0010,
  CHIF_NET_CHECK_EVENT_ERROR = 0x0001,
  CHIF_NET_CHECK_EVENT_CLOSED = 0x0002,
  CHIF_NET_CHECK_EVENT_INVALID = 0x0004
#else
  CHIF_NET_CHECK_EVENT_READ = 0x0001,
  CHIF_NET_CHECK_EVENT_WRITE = 0x0004,
  CHIF_NET_CHECK_EVENT_ERROR = 0x0008,
  CHIF_NET_CHECK_EVENT_CLOSED = 0x0010,
  CHIF_NET_CHECK_EVENT_INVALID = 0x0020
#endif

} chif_net_check_event;

// ====================================================================== //
// Definition
// ====================================================================== //

/**
 * Called to start up the chif network library.
 * Only required when using winsock library.
 *
 * @return Result of the operation
 */
chif_net_result
chif_net_startup();

/**
 * Called to shut down the chif network library.
 * Only required when using winsock library.
 *
 * @return Result of the operation.
 */
chif_net_result
chif_net_shutdown();
/**
 * Open a socket that uses the specified transport protocol for data
 * transmission.
 *
 * @param socket_out
 * @param transport_protocol
 * @param address_family
 * @return
 */
chif_net_result
chif_net_open_socket(chif_net_socket* socket_out,
                     const chif_net_transport_protocol transport_protocol,
                     const chif_net_address_family address_family);

/**
 * Closes a socket that was previously opened with the open socket function.
 *
 * @param socket_out
 * @return
 */
chif_net_result
chif_net_close_socket(chif_net_socket* socket_out);

/**
 * Connect to an address.
 *
 * @pre Make sure @socket is open (call chif_net_open_socket).
 * @param socket
 * @param address
 * @return
 */
chif_net_result
chif_net_connect(const chif_net_socket socket, const chif_net_address* address);

/**
 * Bind a socket to the port on address localhost.
 *
 * @param socket
 * @param address
 * @return
 */
chif_net_result
chif_net_bind(const chif_net_socket socket, const chif_net_address* address);

/**
 * Start listening for connections on a socket.
 *
 * @param socket
 * @param maximum_backlog Queue length for sockets waiting to be accepted.
 *                        Use CHIF_NET_DEFAULT_BACKLOG for default.
 * @return
 */
chif_net_result
chif_net_listen(const chif_net_socket socket, const int maximum_backlog);

/**
 * Accept extracts the first pending connection request on the given
 * listening socket. Can be used with CHIF_NET_TRANSPORT_PROTOCOL_TCP.
 *
 * @pre Allocate: Ensure you allocate the appropriate amount of memory for
 * client_address_out. Size of the different address structure may differ.
 * @pre Set address family: The address family field in the client_address_out.
 * must be filled out to the correct value.
 * @param listening_socket
 * @param client_address_out
 * @param client_socket_out
 * @return
 */
chif_net_result
chif_net_accept(const chif_net_socket listening_socket,
                chif_net_address* client_address_out,
                chif_net_socket* client_socket_out);

/**
 * Read data from the socket. Will block if blocking is set and cannot read.
 * If supplied buffer is smaller than the data available,
 * CHIF_NET_TRANSPORT_PROTOCOL_UDP will discard the remaining data.
 * However, CHIF_NET_TRANSPORT_PROTOCOL_TCP will not.
 *
 * Note, read_bytes of value 0 may indicate connection closed if using TCP.
 * But can also mean that a packet of 0 length was received.
 *
 * @param socket
 * @param buf_out
 * @param bufsize
 * @param read_bytes_out May be NULL if you don't want the data.
 * @return
 */
chif_net_result
chif_net_read(const chif_net_socket socket,
              uint8_t* buf_out,
              const size_t bufsize,
              int* read_bytes_out);

/**
 * Like chif_net_read, but places the source addr of the message in srcaddr.
 *
 * @pre Allocate: Ensure you allocate the appropriate amount of memory for
 * from_address_out. Size of the different address structure may differ.
 * @pre Set address family: The address family field in the from_address_out.
 * must be filled out to the correct value.
 * @param socket
 * @param buf_out
 * @param bufsize
 * @param read_bytes_out May be NULL if you don't want the data.
 * @param from_address_out
 * @return
 */
chif_net_result
chif_net_readfrom(const chif_net_socket socket,
                  uint8_t* buf_out,
                  const size_t bufsize,
                  int* read_bytes_out,
                  chif_net_address* from_address_out);

/**
 * Write to a socket. Will block if buffer does not fit in the send buffer.
 * Unless nonblock io mode is set, then it will return error. You can use
 * chif_net_can_write to check if the interface is ready to write more data.
 *
 * @param socket
 * @param buf
 * @param bufsize
 * @param sent_bytes_out May be NULL if you don't want the data.
 * @return
 */
chif_net_result
chif_net_write(const chif_net_socket socket,
               const uint8_t* buf,
               const size_t bufsize,
               int* sent_bytes_out);

/**
 * Write to a socket, just as chif_net_write, but has a target address option.
 * @param socket
 * @param buf
 * @param bufsize
 * @param sent_bytes_out May be NULL if you don't want the data.
 * @param to_address
 * @return
 */
chif_net_result
chif_net_writeto(const chif_net_socket socket,
                 const uint8_t* buf,
                 const size_t bufsize,
                 int* sent_bytes_out,
                 const chif_net_address* to_address);

/**
 * Check a/multiple socket(s) for events such as
 *
 * CHIF_NET_CHECK_EVENT_READ - can the socket read without blocking?
 * CHIF_NET_CHECK_EVENT_WRITE - can the socket write without blocking? (Given
 * that we don't write more than the socket can handle.)
 * CHIF_NET_CHECK_EVENT_ERROR - does the socket have any error?
 *
 * @param check Check struct(s).
 * @param check_count How many check structures check has.
 * @param read_count_out How many of the check structs that has events.
 * NOTE: a value of 0 means the function timed out without any socket
 * having an event.
 * @param timeout_ms Maximum amount of time the call can wait before returning.
 */
chif_net_result
chif_net_poll(chif_net_check* check,
              const size_t check_count,
              int* ready_count_out,
              const int timeout_ms);

/**
 * Is there any data waiting to be read?
 *
 * Note: If the socket is in a listening state, it will instead check if
 * there is any pending connection waiting to be accepted.
 *
 * See chif_net_get_bytes_available to get amount of bytes that can be read.
 *
 * @param socket
 * @param can_read_out
 * @param timeout_ms How long should we wait before accepting a negative
 * response?
 * @return
 */
chif_net_result
chif_net_can_read(const chif_net_socket socket,
                  int* can_read_out,
                  const int timeout_ms);

/**
 * Can we write data?
 *
 * @param socket
 * @param can_write_out
 * @param timeout_ms How long should we wait before accepting a negative
 * response?
 * @return
 */
chif_net_result
chif_net_can_write(const chif_net_socket socket,
                   int* can_write_out,
                   const int timeout_ms);

/**
 * Check if the socket has any errors. This includes detecting a (cleanly)
 * closed TCP connection.
 *
 * @param socket
 * @param timeout_ms How long should we wait before accepting a negative
 * response?
 * @return If no error, CHIF_NET_RESULT_SUCCESS will be returned.
 */
chif_net_result
chif_net_has_error(const chif_net_socket socket, const int timeout_ms);

/**
 * Sets the blocking mode of a socket to either blocking or non-blocking
 * depending on the specified flag.
 *
 * @param socket Socket to set blocking mode on.
 * @param blocking True to make operations on the socket blocking. False to
 * make operations on the socket non-blocking.
 * @return Result of the operation.
 */
chif_net_result
chif_net_set_blocking(const chif_net_socket socket,
                      const chif_net_bool blocking);

/**
 * Fill in address from information. If needed, will automagically find the
 * address by doing DNS lookup, etc.
 *
 * @pre Ensure you allocate the appropriate amount of memory for
 * address_out. Size of the different address structure may differ.
 * @pre Both name and service cannot be CHIF_NET_ANY_ADDRESS and
 * CHIF_NET_ANY_PORT.
 * @param address_out Output address
 * @param name Example, "127.0.0.1" or "www.duckduckgo.com" or "localhost".
 * @param service Example, "http" or "80". May use CHIF_NET_ANY_PORT
 * @param address_family
 * @param transport_protocol
 * @return
 */
chif_net_result
chif_net_create_address(chif_net_address* address_out,
                        const char* name,
                        const char* service,
                        const chif_net_address_family address_family,
                        const chif_net_transport_protocol transport_protocol);

/**
 * Get the address of a socket.
 *
 * @pre Ensure you allocate the appropriate amount of memory for
 * address_out. Size of the different address structure may differ.
 * @param socket
 * @param address_out
 * @return
 */
chif_net_result
chif_net_address_from_socket(const chif_net_socket socket,
                             chif_net_address* address_out);

/**
 * Get the address of the peer that the socket is connected to.
 *
 * @pre Ensure you allocate the appropriate amount of memory for
 * peer_address_out. Size of the different address structure may differ.
 * @param socket
 * @param peer_address_out
 * @return
 */
chif_net_result
chif_net_peer_address_from_socket(const chif_net_socket socket,
                                  chif_net_address* peer_address_out);

/**
 * From a socket, get the IP address of it as a string.
 * ipv4 -> "XXX.XXX.XXX.XXX"
 * ipv6 -> "XX:XX:XX:XX:XX:XX"
 *
 * @param socket
 * @param str_out
 * @param strlen Should be at least CHIF_NET_IPV4_STRING_LENGTH for ipv4,
 * and CHIF_NET_IPV6_STRING_LEGNTH for ipv6. Use CHIF_NET_IPVX_STRING_LENGTH
 * to contain either.
 * @return
 */
chif_net_result
chif_net_ip_from_socket(const chif_net_socket socket,
                        char* str_out,
                        const size_t strlen);

/**
 * From an address, get the IP address of it as a string.
 * ipv4 -> "XXX.XXX.XXX.XXX"
 * ipv6 -> "XX:XX:XX:XX:XX:XX"
 *
 * @param socket
 * @param str_out
 * @param strlen Should be at least CHIF_NET_IPV4_STRING_LENGTH for ipv4, and
 * CHIF_NET_IPV6_STRING_LEGNTH for ipv6. Use CHIF_NET_IPVX_STRING_LENGTH to
 * contain either.
 * @return
 */
chif_net_result
chif_net_ip_from_address(const chif_net_address* address,
                         char* str_out,
                         const size_t strlen);

/**
 * From a socket, get the port of it.
 *
 * @param socket
 * @param port_out
 * @return
 */
chif_net_result
chif_net_port_from_socket(const chif_net_socket socket,
                          chif_net_port* port_out);

/**
 * From an address, get the port of it.
 *
 * @param address
 * @param port_out
 * @return
 */
chif_net_result
chif_net_port_from_address(const chif_net_address* address,
                           chif_net_port* port_out);

/**
 * Get number of bytes available for read on given socket.
 *
 * @param socket
 * @param bytes_available Amount of bytes available to read.
 * @return
 */
chif_net_result
chif_net_get_bytes_available(const chif_net_socket socket,
                             unsigned long* bytes_available_out);

/**
 * Allow reuse of addresses. Best used before calling bind.
 *
 * @param socket
 * @param reuse Boolean option
 * @return
 */
chif_net_result
chif_net_set_reuse_addr(const chif_net_socket socket,
                        const chif_net_bool reuse);

/**
 * Allow to reuse ports.
 * Note: Not possible on windows platform.
 *
 * @param socket
 * @param reuse To reuse or not, 0 for no, 1 for yes.
 * @return
 */
chif_net_result
chif_net_set_reuse_port(const chif_net_socket socket,
                        const chif_net_bool reuse);

/**
 * Set the connection to keep it alive, if supported by the protocol.
 * Useless for connectionless protocols such as UDP.
 *
 * @param socket
 * @param keepalive
 * @return
 */
// TODO test this
chif_net_result
chif_net_set_keepalive(const chif_net_socket socket,
                       const chif_net_bool keepalive);

/**
 * Enables broadcast privileges, if supported by the protocol. Typically
 * used with the UDP protocol.
 * @param socket
 * @param broadcast
 * @return
 */
// TODO test this
chif_net_result
chif_net_set_broadcast(const chif_net_socket socket,
                       const chif_net_bool broadcast);

/**
 * Set the timeout for blocking receive calls.
 *
 * @param socket
 * @param time_ms
 * @return
 */
// TODO test this
chif_net_result
chif_net_set_recv_timeout(const chif_net_socket socket, const int time_ms);

/**
 * Set the timeout for blocking send calls.
 *
 * @param socket
 * @param time_ms
 * @return
 */
// TODO test this
chif_net_result
chif_net_set_send_timeout(const chif_net_socket socket, const int time_ms);

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
 *
 * @param socket
 * @param ms Time in milliseconds that we wait before timeout
 * @return
 */
// TODO test this
chif_net_result
chif_net_tcp_set_user_timeout(const chif_net_socket socket, const int time_ms);

/* chif_net_result */
/* chif_net_tcp_get_user_timeout(chif_net_socket socket, int time_ms); */

/**
 * If set, disable the Nagle algorithm. This means that segments are always
 * sent out as soon as possible, even if there is only a small amount of data.
 *
 * @param socket
 * @param nodelay
 * @return
 */
// TODO test this
chif_net_result
chif_net_tcp_set_nodelay(const chif_net_socket socket,
                         const chif_net_bool nodelay);

/**
 * Set the number of SYN retransmits that TCP should send before aborting the
 * attempt to connect.
 *
 * @param socket
 * @param count On the range [0-255]
 * @return
 */
chif_net_result
chif_net_tcp_set_syncnt(const chif_net_socket socket, const int count);

/**
 * Set the time to live (ttl) parameter in the IP header. This value will
 * determine how many routers the packet can hop through.
 *
 * @param sock
 * @param ttl
 * @return
 */
chif_net_result
chif_net_set_ttl(const chif_net_socket socket, const int ttl);

/**
 * Do we want to provide our own ip header?
 *
 * @param sock
 * @param provide_own_hdr 0 means no, 1 means yes, the user will build hdr.
 * @return
 */
chif_net_result
chif_net_set_own_iphdr(const chif_net_socket socket, const int provide_own_hdr);

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
// TODO remove?
/* chif_net_result chif_net_icmp_build(uint8_t* buf, */
/*                                     size_t* bufsize, */
/*                                     const void* data, */
/*                                     size_t data_size, */
/*                                     uint16_t id, */
/*                                     uint16_t seq); */

/**
 * Convert the enumerated result to a string, good for printing the result.
 * @param result
 * @return Pointer to the string.
 */
const char*
chif_net_result_to_string(const chif_net_result result);

/**
 * @param address_family
 * @return
 */
const char*
chif_net_address_family_to_string(const chif_net_address_family address_family);

/**
 * @param transport_protocol
 * @return
 */
const char*
chif_net_transport_protocol_to_string(
  const chif_net_transport_protocol transport_protocol);

#if defined(__cplusplus)
}
#endif

#endif // CHIF_NET_H
