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

// ============================================================ //
// Headers
// ============================================================ //

#include "chif_net.h"

#if defined(CHIF_NET_BERKLEY_SOCKET)
#include <netdb.h>
#endif

// ============================================================ //
// Static functions
// ============================================================ //

static CHIF_NET_INLINE chif_net_result
_chif_net_get_specific_result_type()
{
#if defined(CHIF_NET_WINSOCK2)
  const DWORD error = GetLastError();
  // TODO handle more errors on windows
  switch (error) {
    case WSANOTINITIALISED:
      return CHIF_NET_RESULT_LIBRARY_NOT_INITIALIZED;

    case WSAEMFILE:
      return CHIF_NET_RESULT_MAX_SOCKETS_REACHED;

    case WSAENOTSOCK:
      return CHIF_NET_RESULT_NOT_A_SOCKET;

    case WSAEAFNOSUPPORT:
      return CHIF_NET_RESULT_INVALID_ADDRESS;

    case WSAEACCES:
      return CHIF_NET_RESULT_ACCESS_DENIED;

    case WSAEADDRINUSE:
      return CHIF_NET_RESULT_SOCKET_ALREADY_IN_USE;

    case WSAEADDRNOTAVAIL:
      return CHIF_NET_RESULT_INVALID_ADDRESS;

    case WSAEINPROGRESS:
      return CHIF_NET_RESULT_IN_PROGRESS;

    case WSAENOBUFS:
      return CHIF_NET_RESULT_NOT_ENOUGH_SPACE;

    case WSAENETDOWN:
      return CHIF_NET_RESULT_NETWORK_SUBSYSTEM_FAILED;

    case WSAEFAULT:
      return CHIF_NET_RESULT_INVALID_INPUT_PARAM;

    case WSAEINTR:
      return CHIF_NET_RESULT_BLOCKING_CANCELED;

    case WSAEALREADY:
      return CHIF_NET_RESULT_IN_PROGRESS;

    case WSAEINVAL:
      return CHIF_NET_RESULT_IN_PROGRESS;

    case WSAEISCONN:
      return CHIF_NET_RESULT_ALREADY_CONNECTED;

    case WSAENETUNREACH:
    case WSAEHOSTUNREACH:
      return CHIF_NET_RESULT_NET_UNREACHABLE;

    case WSAETIMEDOUT:
      return CHIF_NET_RESULT_TIMEDOUT;

    case WSAEWOULDBLOCK:
      return CHIF_NET_RESULT_WOULD_BLOCK;

    case WSAECONNREFUSED:
      return CHIF_NET_RESULT_CONNECTION_REFUSED;

    default:
      return CHIF_NET_RESULT_UNKNOWN;
  }

#elif defined(CHIF_NET_BERKLEY_SOCKET)
  switch (errno) {
    case ENOTSOCK:
      return CHIF_NET_RESULT_NOT_A_SOCKET;

    case EBADF:
      return CHIF_NET_RESULT_INVALID_FILE_DESCRIPTOR;

    case EALREADY:
      return CHIF_NET_RESULT_WOULD_BLOCK;

    case ECONNREFUSED:
      return CHIF_NET_RESULT_CONNECTION_REFUSED;

    case EACCES:
      return CHIF_NET_RESULT_ACCESS_DENIED;

    case EADDRINUSE:
      return CHIF_NET_RESULT_SOCKET_ALREADY_IN_USE;

    case EAGAIN:
      return CHIF_NET_RESULT_NO_FREE_PORT;

    case EISCONN:
      return CHIF_NET_RESULT_ALREADY_CONNECTED;

    case EINPROGRESS:
      return CHIF_NET_RESULT_IN_PROGRESS;

    case ETIMEDOUT:
      return CHIF_NET_RESULT_TIMEDOUT;

    case ECONNABORTED:
      return CHIF_NET_RESULT_CONNECTION_ABORTED;

    case EINVAL:
      return CHIF_NET_RESULT_NOT_LISTENING_OR_NOT_CONNECTED;

    case EMFILE:
      return CHIF_NET_RESULT_NO_FREE_FILE_DESCRIPTORS;

    case ENFILE:
      return CHIF_NET_RESULT_NO_FREE_FILES;

    case ECONNRESET:
      return CHIF_NET_RESULT_SOCKET_RESET;

    case ENOTCONN:
      return CHIF_NET_RESULT_CONNECTION_CLOSED;

    case EAFNOSUPPORT:
      return CHIF_NET_RESULT_NOT_VALID_ADDRESS_FAMILY;

    case ENOSPC:
      return CHIF_NET_RESULT_NOT_ENOUGH_SPACE;

    case EPIPE:
      return CHIF_NET_RESULT_CONNECTION_CLOSED;

    case EMSGSIZE:
      return CHIF_NET_RESULT_TOO_LONG_MSG_NOT_SENT;

    case ENOBUFS:
    case ENOMEM:
      return CHIF_NET_RESULT_NO_MEMORY;

    case EPROTONOSUPPORT:
      return CHIF_NET_RESULT_INVALID_PROTOCOL;

    case EPERM:
      return CHIF_NET_RESULT_ACCESS_DENIED;

    case ENETUNREACH:
      return CHIF_NET_RESULT_NO_NETWORK;

    default:
      return CHIF_NET_RESULT_UNKNOWN;
  }
#else
  return CHIF_NET_RESULT_UNKNOWN;
#endif
}

/**
 * @pre optval must be int for boolean operations
 * @param socket
 * @param level
 * @param optname
 * @param optval Must be int for boolean operations
 * @param optlen
 * @return
 */
static CHIF_NET_INLINE chif_net_result
_chif_net_setsockopt(chif_net_socket socket,
                     int level,
                     int optname,
                     const void* optval,
                     socklen_t optlen)
{
  int res_not_ok;

#if defined(CHIF_NET_BERKLEY_SOCKET) || defined(CHIF_NET_WINSOCK2)
  res_not_ok = setsockopt(socket, level, optname, (const char*)optval, optlen);
#else
  return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif

  if (res_not_ok) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

/**
 * Use the syscall poll to check if the given events has happened.
 *
 * @TODO poll
 * [ ] User can poll several devices.
 * [x] User can set a timeout.
 * [ ] Implement a Windows version.
 *
 * @param socket Socket to check.
 * @param res Output parameter, 0 for fail, or 1 for success.
 * @param events Check man page for poll for the possible events.
 * @param timeout_ms If event being polled has not happened, how long before
 *                   we should timeout and return? Use 0 to return instantly.
 * @return The result of the syscall translated into chif_net_result types.
 */
static CHIF_NET_INLINE chif_net_result
_chif_net_poll(chif_net_socket socket, int* res, short events, int timeout_ms)
{
  struct pollfd pfd = { socket, events, 0 };
  nfds_t nfds = 1;

#if defined(CHIF_NET_WINSOCK2)
  int pres = WSAPoll(&pfd, nfds, timeout_ms);
#else // if defined(CHIF_NET_BERKLEY_SOCKET)
  int pres = poll(&pfd, nfds, timeout_ms);
#endif

  if (pres == 0) {
    *res = 0;
  } else if (pres > 0) {
    if (pfd.revents & events) {
      *res = 1;
    } else {
      *res = 0;
      if (pfd.revents & POLLERR) {
        return CHIF_NET_RESULT_FAIL;
      } else if (pfd.revents & POLLHUP) {
        return CHIF_NET_RESULT_CONNECTION_CLOSED;
      } else if (pfd.revents & POLLNVAL) {
        return CHIF_NET_RESULT_INVALID_FILE_DESCRIPTOR;
      }
    }
  } else {
    *res = 0;
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

/**
 * Calculate a 0's compliment checksum, commonly used for network checksum.
 * @param buf
 * @param buf_size
 */
static CHIF_NET_INLINE uint16_t
_chif_net_0s_checksum(uint8_t* buf, size_t bufsize)
{
  uint32_t sum = 0;
  uint16_t ret = 0;
  uint16_t* ptr;

  for (ptr = (uint16_t*)buf; bufsize > 1; ptr++, bufsize -= 2)
    sum += *ptr;

  if (bufsize == 1) {
    *(char*)&ret = *(char*)ptr;
    sum += ret;
  }

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum = (sum >> 16) + (sum & 0xFFFF);

  ret = (uint16_t)~sum;

  return ret;
}

// ====================================================================== //
// Implementation
// ====================================================================== //

CHIF_NET_INLINE chif_net_result
chif_net_startup()
{
#if defined(CHIF_NET_WINSOCK2)
  WSADATA winsock_data;
  const int32_t result = WSAStartup(WINSOCK_VERSION, &winsock_data);
  if (result != NO_ERROR) {
    return CHIF_NET_RESULT_FAIL;
  }
#endif
  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_shutdown()
{
#if defined(CHIF_NET_WINSOCK2)
  const int32_t result = WSACleanup();
  if (result == WSAEINPROGRESS)
    return CHIF_NET_RESULT_BLOCKING;
  else if (result != 0)
    return CHIF_NET_RESULT_FAIL;
#endif
  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_open_socket(chif_net_socket* socket_out,
                     chif_net_protocol transport_protocol,
                     chif_net_address_family address_family)
{
  int addrfam;
  switch (address_family) {
    case CHIF_NET_ADDRESS_FAMILY_IPV4: {
      addrfam = AF_INET;
      break;
    }
    case CHIF_NET_ADDRESS_FAMILY_IPV6: {
      addrfam = AF_INET6;
      break;
    }
    case CHIF_NET_ADDRESS_FAMILY_UNIX: {
      addrfam = AF_UNIX;
      break;
    }
  }

  int tproto;
  int ipproto;
  switch (transport_protocol) {
    case CHIF_NET_PROTOCOL_TCP: {
      tproto = SOCK_STREAM;
      ipproto = IPPROTO_TCP;
      break;
    }
    case CHIF_NET_PROTOCOL_UDP: {
      tproto = SOCK_DGRAM;
      ipproto = IPPROTO_UDP;
      break;
    }
    case CHIF_NET_PROTOCOL_ICMP: {
      tproto = SOCK_RAW;
      if (address_family == CHIF_NET_ADDRESS_FAMILY_IPV4) {
        ipproto = IPPROTO_ICMP;
      } else { // if (address_family == CHIF_NET_ADDRESS_FAMILY_IPV6) {
        ipproto = IPPROTO_ICMPV6;
      }
      break;
    }
    case CHIF_NET_PROTOCOL_RAW: {
      tproto = SOCK_RAW;
      ipproto = IPPROTO_RAW;
      break;
    }
  }

  const chif_net_socket result_socket = socket(addrfam, tproto, ipproto);

  if (result_socket == CHIF_NET_INVALID_SOCKET) {
    *socket_out = CHIF_NET_INVALID_SOCKET;
    return _chif_net_get_specific_result_type();
  }

  *socket_out = result_socket;

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_close_socket(chif_net_socket* socket)
{
  if (*socket != CHIF_NET_INVALID_SOCKET) {
    // Close the socket
#if defined(CHIF_NET_WINSOCK2)
    const int result = closesocket(*socket);
#elif defined(CHIF_NET_BERKLEY_SOCKET)
    int result = close(*socket);
#else
    return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif

    // Set socket to invalid to prevent usage of closed socket.
    *socket = CHIF_NET_INVALID_SOCKET;

    if (result == CHIF_NET_SOCKET_ERROR)
      return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_connect(chif_net_socket socket, chif_net_address* address)
{
  const int result =
    connect(socket,
            (struct sockaddr*)&address->addr,
            address->addr.ss_family == AF_INET ? sizeof(struct sockaddr_in)
                                               : sizeof(struct sockaddr_in6));

  if (result == CHIF_NET_SOCKET_ERROR)
    return _chif_net_get_specific_result_type();

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_bind(chif_net_socket socket,
              chif_net_port port,
              chif_net_address_family address_family)
{
  chif_net_address address;
  // TODO address construction without string
  int result =
    chif_net_create_address(&address, "0.0.0.0", port, address_family);

  if (result == CHIF_NET_SOCKET_ERROR)
    return _chif_net_get_specific_result_type();

  result = bind(socket, (struct sockaddr*)&address.addr, sizeof(address.addr));

  if (result == CHIF_NET_SOCKET_ERROR)
    return _chif_net_get_specific_result_type();

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_listen(chif_net_socket socket, int maximum_backlog)
{
  const int result = listen(socket, maximum_backlog);

  if (result == CHIF_NET_SOCKET_ERROR)
    return _chif_net_get_specific_result_type();

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_accept(chif_net_socket server_socket,
                chif_net_address* client_address,
                chif_net_socket* client_socket)
{
  // TODO set client_address.sin_family
  socklen_t client_addrlen = sizeof(struct sockaddr_in);

  *client_socket = accept(
    server_socket, (struct sockaddr*)&client_address->addr, &client_addrlen);

  if (*client_socket == CHIF_NET_INVALID_SOCKET) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_read(chif_net_socket socket,
              uint8_t* buf,
              size_t bufsize,
              ssize_t* read_bytes)
{
  if (socket == CHIF_NET_INVALID_SOCKET)
    return CHIF_NET_RESULT_NOT_A_SOCKET;

  int flag = 0;
#if defined(CHIF_NET_BERKLEY_SOCKET)
  // Prevent SIGPIPE signal and handle the error in application code
  flag = MSG_NOSIGNAL;
#endif

  const ssize_t result = recv(socket, (char*)buf, (int)bufsize, flag);

  if (result == CHIF_NET_SOCKET_ERROR)
    return _chif_net_get_specific_result_type();
  else if (!result)
    return CHIF_NET_RESULT_CONNECTION_CLOSED;

  if (read_bytes)
    *read_bytes = result;

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_readfrom(chif_net_socket socket,
                  uint8_t* buf,
                  size_t bufsize,
                  ssize_t* read_bytes,
                  chif_net_address* srcaddr)
{
  if (socket == CHIF_NET_INVALID_SOCKET)
    return CHIF_NET_RESULT_NOT_A_SOCKET;

  int flag = 0;
#if defined(CHIF_NET_BERKLEY_SOCKET)
  // Prevent SIGPIPE signal and handle the error in application code
  flag = MSG_NOSIGNAL;
#endif

  socklen_t srcaddr_size = sizeof(struct sockaddr);

  // buflen param is diffrent type in winsock2 lib
#if defined(CHIF_NET_WINSOCK2)
  if (bufsize > INT_MAX)
    return CHIF_NET_RESULT_BUFSIZE_INVALID;
#define READFROM_BUFSIZE (int)bufsize
#else
#define READFROM_BUFSIZE bufsize
#endif

  const ssize_t result = recvfrom(socket,
                                  (char*)buf,
                                  READFROM_BUFSIZE,
                                  flag,
                                  (struct sockaddr*)srcaddr,
                                  &srcaddr_size);
#undef READFROM_BUFSIZE

  if (result == CHIF_NET_SOCKET_ERROR)
    return _chif_net_get_specific_result_type();
  else if (!result)
    return CHIF_NET_RESULT_CONNECTION_CLOSED;

  if (read_bytes)
    *read_bytes = result;

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_write(chif_net_socket socket,
               const uint8_t* buf,
               size_t bufsize,
               ssize_t* sent_bytes)
{
  if (socket == CHIF_NET_INVALID_SOCKET)
    return CHIF_NET_RESULT_NOT_A_SOCKET;

  int flag = 0;
#if defined(CHIF_NET_BERKLEY_SOCKET)
  // Prevent SIGPIPE signal handle the error in application code
  flag = MSG_NOSIGNAL;
#endif

  const ssize_t result = send(socket, (const char*)buf, (int)bufsize, flag);

  if (result == CHIF_NET_SOCKET_ERROR)
    return _chif_net_get_specific_result_type();

  if (sent_bytes)
    *sent_bytes = result;

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_writeto(chif_net_socket socket,
                 const uint8_t* buf,
                 size_t bufsize,
                 ssize_t* sent_bytes,
                 chif_net_address* target_addr)
{
  if (socket == CHIF_NET_INVALID_SOCKET)
    return CHIF_NET_RESULT_NOT_A_SOCKET;

  int flag = 0;
#if defined(CHIF_NET_BERKLEY_SOCKET)
  // Prevent SIGPIPE signal handle the error in application code
  flag = MSG_NOSIGNAL;
#endif

  const ssize_t result = sendto(socket,
                                (const char*)buf,
                                (int)bufsize,
                                flag,
                                (struct sockaddr*)target_addr,
                                sizeof(struct sockaddr));

  if (result == CHIF_NET_SOCKET_ERROR)
    return _chif_net_get_specific_result_type();

  if (sent_bytes)
    *sent_bytes = result;
  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_can_read(chif_net_socket socket, int* socket_can_read, int timeout_ms)
{
  return _chif_net_poll(socket, socket_can_read, POLLIN, timeout_ms);
}

CHIF_NET_INLINE chif_net_result
chif_net_can_write(chif_net_socket socket,
                   int* socket_can_write,
                   int timeout_ms)
{
  return _chif_net_poll(socket, socket_can_write, POLLOUT, timeout_ms);
}

CHIF_NET_INLINE chif_net_result
chif_net_has_error(chif_net_socket socket)
{
  int socket_no_error;
  const chif_net_result res = _chif_net_poll(socket, &socket_no_error, 0, 0);
  return res;
}

CHIF_NET_INLINE chif_net_result
chif_net_set_socket_blocking(chif_net_socket socket, bool blocking)
{
#if defined(CHIF_NET_WINSOCK2)
  u_long blocking_mode = !blocking;
  int result = ioctlsocket(socket, (long)FIONBIO, &blocking_mode);
  if (result == CHIF_NET_SOCKET_ERROR) {
    return _chif_net_get_specific_result_type();
  }
#elif defined(CHIF_NET_BERKLEY_SOCKET)
  int flags = fcntl(socket, F_GETFL, 0);
  if (blocking)
    flags &= ~O_NONBLOCK;
  else
    flags |= O_NONBLOCK;
  fcntl(socket, F_SETFL, flags);
#endif

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_create_address(chif_net_address* address_out,
                        const char* ip_address,
                        chif_net_port port,
                        chif_net_address_family address_family)
{
  // TODO support UNIX domain sockets
  int result;

  if (address_family == CHIF_NET_ADDRESS_FAMILY_IPV4) {
    address_out->addr.ss_family = AF_INET;
    struct sockaddr_in* addr_in = (struct sockaddr_in*)(&address_out->addr);
    result =
      inet_pton((*address_out).addr.ss_family, ip_address, &addr_in->sin_addr);
    (*addr_in).sin_port = htons(port);
  } else if (address_family == CHIF_NET_ADDRESS_FAMILY_IPV6) {
    address_out->addr.ss_family = AF_INET6;
    struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)(&address_out->addr);
    result = inet_pton(
      (*address_out).addr.ss_family, ip_address, &addr_in6->sin6_addr);
    (*addr_in6).sin6_port = htons(port);
  } else {
    return CHIF_NET_RESULT_NOT_VALID_ADDRESS_FAMILY;
  }

  if (result != 1) {
    if (result == -1) {
      return _chif_net_get_specific_result_type();
    } else if (result == 0) {
      return CHIF_NET_RESULT_INVALID_ADDRESS;
    }
    return CHIF_NET_RESULT_UNKNOWN;
  }

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_get_address(chif_net_socket socket, chif_net_address* address)
{
  struct sockaddr* addr = (struct sockaddr*)&address->addr;
  socklen_t addrlen = sizeof(chif_net_address);
  const int result = getsockname(socket, addr, &addrlen);

  if (addrlen > (socklen_t)sizeof(chif_net_address)) {
    // address was truncated because lack of storage in addr
    return CHIF_NET_RESULT_UNKNOWN;
  }

  if (result) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_get_peer_address(chif_net_socket socket, chif_net_address* address)
{
  struct sockaddr* addr = (struct sockaddr*)&address->addr;
  socklen_t addr_len = sizeof(chif_net_address);
  const int result = getpeername(socket, addr, &addr_len);

  if (addr_len > (socklen_t)sizeof(chif_net_address)) {
    // address was truncated because lack of storage in addr
    return CHIF_NET_RESULT_BUFSIZE_INVALID;
  }

  if (result != 0) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_ip_from_socket(chif_net_socket socket, char* str, size_t strlen)
{
  chif_net_address address;
  chif_net_result result = chif_net_get_address(socket, &address);
  if (result != CHIF_NET_RESULT_SUCCESS)
    return result;

  return chif_net_ip_from_address(&address, str, strlen);
}

CHIF_NET_INLINE chif_net_result
chif_net_ip_from_address(const chif_net_address* address,
                         char* str,
                         size_t strlen)
{
  const char* ntop_result;
  if (address->addr.ss_family == AF_INET) {
    const struct sockaddr_in* addr_ivp4 =
      (const struct sockaddr_in*)&address->addr;
    ntop_result =
      inet_ntop(address->addr.ss_family, &addr_ivp4->sin_addr, str, strlen);
  } else { // if (net_address.addr.ss_family == AF_INET6)
    const struct sockaddr_in6* addr_ivp6 =
      (const struct sockaddr_in6*)&address->addr;
    ntop_result =
      inet_ntop(address->addr.ss_family, &addr_ivp6->sin6_addr, str, strlen);
  }

  if (ntop_result == NULL) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_port_from_socket(chif_net_socket socket, chif_net_port* port)
{
  chif_net_address address;
  const chif_net_result result = chif_net_get_address(socket, &address);
  if (result != CHIF_NET_RESULT_SUCCESS)
    return result;

  return chif_net_port_from_address(&address, port);
}

CHIF_NET_INLINE chif_net_result
chif_net_port_from_address(const chif_net_address* address, chif_net_port* port)
{
  if (address->addr.ss_family == AF_INET) {
    const struct sockaddr_in* addr_ivp4 =
      (const struct sockaddr_in*)&address->addr;
    *port = ntohs(addr_ivp4->sin_port);
  } else if (address->addr.ss_family == AF_INET6) {
    const struct sockaddr_in6* addr_ivp6 =
      (const struct sockaddr_in6*)&address->addr;
    *port = ntohs(addr_ivp6->sin6_port);
  }

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_get_bytes_available(chif_net_socket socket,
                             unsigned long* bytes_available)
{
#if defined(CHIF_NET_WINSOCK2)
  const int result = ioctlsocket(socket, FIONREAD, bytes_available);
#elif defined(CHIF_NET_BERKLEY_SOCKET)
  const int result = ioctl(socket, FIONREAD, &bytes_available);
#else
  return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif

  if (result) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

CHIF_NET_INLINE chif_net_result
chif_net_set_reuse_addr(chif_net_socket socket, chif_net_opt_bool reuse)
{
  assert((reuse == 0 || reuse == 1) && "reuse must be boolean value");

  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
}

CHIF_NET_INLINE chif_net_result
chif_net_set_reuse_port(chif_net_socket socket, chif_net_opt_bool reuse)
{
  assert((reuse == 0 || reuse == 1) && "reuse must be boolean value");

#if defined(CHIF_NET_HAS_TCP_DETAILS)
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_REUSEPORT, (char*)&reuse, sizeof(reuse));
#else
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(reuse);
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(socket);
  return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif
}

CHIF_NET_INLINE chif_net_result
chif_net_set_keepalive(chif_net_socket socket, chif_net_opt_bool keepalive)
{
  assert((keepalive == 0 || keepalive == 1) &&
         "keepalive must be boolean value");

  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepalive, sizeof(keepalive));
}

CHIF_NET_INLINE chif_net_result
chif_net_set_broadcast(chif_net_socket socket, chif_net_opt_bool broadcast)
{
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast));
}

CHIF_NET_INLINE chif_net_result
chif_net_set_recv_timeout(chif_net_socket socket, int time_ms)
{
#if defined(CHIF_NET_BERKLEY_SOCKET)
  struct timeval timeout;
  timeout.tv_sec = time_ms / 1000;
  timeout.tv_usec = (time_ms % 1000) * 1000;
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#elif defined(CHIF_NET_WINSOCK2)
  const DWORD timeout = (DWORD)time_ms;
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#else
  return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif
}

CHIF_NET_INLINE chif_net_result
chif_net_set_send_timeout(chif_net_socket socket, int time_ms)
{
#if defined(CHIF_NET_BERKLEY_SOCKET)
  struct timeval timeout;
  timeout.tv_sec = time_ms / 1000;
  timeout.tv_usec = (time_ms % 1000) * 1000;
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#elif defined(CHIF_NET_WINSOCK2)
  const DWORD timeout = (DWORD)time_ms;
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#else
  return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif
}

CHIF_NET_INLINE chif_net_result
chif_net_tcp_set_user_timeout(chif_net_socket socket, int time_ms)
{
#if defined(CHIF_NET_HAS_TCP_DETAILS)
  return _chif_net_setsockopt(
    socket, IPPROTO_TCP, TCP_USER_TIMEOUT, &time_ms, sizeof(time_ms));
#else
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(socket);
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(time_ms);
  return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif
}

CHIF_NET_INLINE chif_net_result
chif_net_tcp_set_nodelay(chif_net_socket socket, chif_net_opt_bool nodelay)
{
  assert((nodelay == 0 || nodelay == 1) && "nodelay must be boolean value");
  return _chif_net_setsockopt(
    socket, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));
}

CHIF_NET_INLINE chif_net_result
chif_net_tcp_set_syncnt(chif_net_socket socket, chif_net_opt_bool count)
{
#if defined(CHIF_NET_HAS_TCP_DETAILS)
  assert(count < 256 && count >= 0 && "count must be [0 - 255]");
  return _chif_net_setsockopt(
    socket, IPPROTO_TCP, TCP_SYNCNT, &count, sizeof(count));
#else
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(socket);
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(count);
  return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif
}

CHIF_NET_INLINE chif_net_result
chif_net_set_ttl(chif_net_socket sock, int ttl)
{
  return _chif_net_setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
}

CHIF_NET_INLINE chif_net_result
chif_net_set_own_iphdr(chif_net_socket sock, int provide_own_hdr)
{
  return _chif_net_setsockopt(
    sock, IPPROTO_IP, IP_HDRINCL, &provide_own_hdr, sizeof(provide_own_hdr));
}

CHIF_NET_INLINE chif_net_result
chif_net_icmp_build(uint8_t* buf,
                    size_t* bufsize,
                    const void* data,
                    size_t data_size,
                    uint16_t id,
                    uint16_t seq)
{
#if defined(CHIF_NET_BERKLEY_SOCKET) && defined(__linux__)
  const size_t packet_size =
    data_size + sizeof(struct iphdr) + sizeof(struct icmphdr);
  if (*bufsize >= packet_size) {
    struct icmphdr* icmp_hdr = (struct icmphdr*)(buf);
    icmp_hdr->type = ICMP_ECHO;
    icmp_hdr->code = 0; // MUST be 0
    icmp_hdr->un.echo.id = id;
    icmp_hdr->un.echo.sequence = seq;
    icmp_hdr->checksum = 0; // must be zero when calculating actual checksum
    memcpy(buf + sizeof(struct icmphdr), data, data_size);
    icmp_hdr->checksum = _chif_net_0s_checksum(buf, packet_size);

    *bufsize = packet_size;
    return CHIF_NET_RESULT_SUCCESS;
  } else {
    return CHIF_NET_RESULT_NOT_ENOUGH_SPACE;
  }
#else
  (void)buf;
  (void)bufsize;
  (void)data;
  (void)data_size;
  (void)id;
  (void)seq;
  assert(false && "not implemented");
  return CHIF_NET_RESULT_FAIL;
#endif
}

CHIF_NET_INLINE const char*
chif_net_result_to_string(chif_net_result result)
{
  switch (result) {
    case CHIF_NET_RESULT_UNKNOWN:
      return "UNKNOWN";
    case CHIF_NET_RESULT_SUCCESS:
      return "SUCCESS";
    case CHIF_NET_RESULT_LIBRARY_NOT_INITIALIZED:
      return "LIBRARY_NOT_INITIALIZED";
    case CHIF_NET_RESULT_BLOCKING:
      return "BLOCKING";
    case CHIF_NET_RESULT_MAX_SOCKETS_REACHED:
      return "MAX_SOCKETS_REACHED";
    case CHIF_NET_RESULT_NOT_A_SOCKET:
      return "NOT_A_SOCKET";
    case CHIF_NET_RESULT_WOULD_BLOCK:
      return "WOULD_BLOCK";
    case CHIF_NET_RESULT_CONNECTION_REFUSED:
      return "CONNECTION_REFUSED";
    case CHIF_NET_RESULT_INVALID_ADDRESS:
      return "INVALID_ADDRESS";
    case CHIF_NET_RESULT_INVALID_FILE_DESCRIPTOR:
      return "INVALID_FILE_DESCRIPTOR";
    case CHIF_NET_RESULT_ACCESS_DENIED:
      return "ACCESS_DENIED";
    case CHIF_NET_RESULT_SOCKET_ALREADY_IN_USE:
      return "SOCKET_ALREADY_IN_USE";
    case CHIF_NET_RESULT_NO_FREE_PORT:
      return "NO_FREE_PORT";
    case CHIF_NET_RESULT_IN_PROGRESS:
      return "IN_PROGRESS";
    case CHIF_NET_RESULT_ALREADY_CONNECTED:
      return "ALREADY_CONNECTED";
    case CHIF_NET_RESULT_TIMEDOUT:
      return "TIMEDOUT";
    case CHIF_NET_RESULT_CONNECTION_ABORTED:
      return "CONNECTION_ABORTED";
    case CHIF_NET_RESULT_NOT_LISTENING_OR_NOT_CONNECTED:
      return "NOT_LISTENING_OR_NOT_CONNECTED";
    case CHIF_NET_RESULT_NO_FREE_FILE_DESCRIPTORS:
      return "NO_FREE_FILE_DESCRIPTORS";
    case CHIF_NET_RESULT_NO_FREE_FILES:
      return "NO_FREE_FILES";
    case CHIF_NET_RESULT_SOCKET_RESET:
      return "SOCKET_RESET";
    case CHIF_NET_RESULT_CONNECTION_CLOSED:
      return "CONNECTION_CLOSED";
    case CHIF_NET_RESULT_NOT_VALID_ADDRESS_FAMILY:
      return "NOT_VALID_ADDRESS_FAMILY";
    case CHIF_NET_RESULT_NOT_ENOUGH_SPACE:
      return "NOT_ENOUGH_SPACE";
    case CHIF_NET_RESULT_NETWORK_SUBSYSTEM_FAILED:
      return "NETWORK_SUBSYSTEM_FAILED";
    case CHIF_NET_RESULT_INVALID_INPUT_PARAM:
      return "INVALID_INPUT_PARAM";
    case CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED:
      return "PLATFORM_NOT_SUPPORTED";
    case CHIF_NET_RESULT_TOO_LONG_MSG_NOT_SENT:
      return "TOO_LONG_MSG_NOT_SENT";
    case CHIF_NET_RESULT_FAIL:
      return "FAIL";
    case CHIF_NET_RESULT_INVALID_PROTOCOL:
      return "INVALID_PROTOCOL";
    case CHIF_NET_RESULT_NO_MEMORY:
      return "NO_MEMORY";
    case CHIF_NET_RESULT_NO_NETWORK:
      return "NO_NETWORK";
    case CHIF_NET_RESULT_BLOCKING_CANCELED:
      return "BLOCKING_CANCELED";
    case CHIF_NET_RESULT_NET_UNREACHABLE:
      return "NET_UNREACHABLE";
    case CHIF_NET_RESULT_BUFSIZE_INVALID:
      return "CHIF_NET_RESULT_BUFSIZE_INVALID";
  }

  // should never happen
  return "INTERNAL_CHIF_ERROR";
}
