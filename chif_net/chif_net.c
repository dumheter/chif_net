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

// ============================================================ //
// Headers
// ============================================================ //

#include "chif_net.h"

#if defined(_WIN32) || defined(_WIN64)
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <winerror.h>
#include <ws2def.h>
#pragma comment(lib, "Ws2_32.lib")
#elif defined(__linux__) || defined(__APPLE__) || defined(__GNUC__)
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#if defined(__linux__)
#include <linux/icmp.h>
#endif

#include <assert.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>
#if defined(CHIF_NET_BERKLEY_SOCKET)
#include <netdb.h>
#endif

// ============================================================ //
// Types
// ============================================================ //

#if defined(CHIF_NET_WINSOCK2)
typedef TIMEVAL timeval;
typedef unsigned long nfds_t;
#endif

// ============================================================ //
// Static Asserts
// ============================================================ //

CHIF_NET_STATIC_ASSERT((int64_t)CHIF_NET_TRANSPORT_PROTOCOL_TCP ==
                         (int64_t)IPPROTO_TCP,
                       ipproto_tcp_correct_value);
CHIF_NET_STATIC_ASSERT((int64_t)CHIF_NET_TRANSPORT_PROTOCOL_UDP ==
                         (int64_t)IPPROTO_UDP,
                       ipproto_udp_correct_value);

CHIF_NET_STATIC_ASSERT((int64_t)CHIF_NET_ADDRESS_FAMILY_IPV4 ==
                         (int64_t)AF_INET,
                       af_ipv4_correct_value);
CHIF_NET_STATIC_ASSERT((int64_t)CHIF_NET_ADDRESS_FAMILY_IPV6 ==
                         (int64_t)AF_INET6,
                       af_ipv6_correct_value);

CHIF_NET_STATIC_ASSERT(sizeof(chif_net_ipv6_address) ==
                         sizeof(struct sockaddr_in6),
                       ipv6_address_struct_correct_size);

CHIF_NET_STATIC_ASSERT((int64_t)CHIF_NET_CHECK_EVENT_WRITE == POLLOUT,
                       check_write_correct_value);
CHIF_NET_STATIC_ASSERT((int64_t)CHIF_NET_CHECK_EVENT_READ == POLLIN,
                       check_read_correct_value);
CHIF_NET_STATIC_ASSERT((int64_t)CHIF_NET_CHECK_EVENT_ERROR == POLLERR,
                       check_error_correct_value);
CHIF_NET_STATIC_ASSERT((int64_t)CHIF_NET_CHECK_EVENT_CLOSED == POLLHUP,
                       check_closed_correct_value);
CHIF_NET_STATIC_ASSERT((int64_t)CHIF_NET_CHECK_EVENT_INVALID == POLLNVAL,
                       check_invalid_correct_value);

CHIF_NET_STATIC_ASSERT(sizeof(chif_net_check) == sizeof(struct pollfd),
                       check_struct_correct_size);

// ============================================================ //
// Static functions
// ============================================================ //

static chif_net_result
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

    case EAI_FAIL:
    case EAI_AGAIN:
      return CHIF_NET_RESULT_NAME_SERVER_FAIL;

    case EAI_MEMORY:
      return CHIF_NET_RESULT_NO_MEMORY;

    case EAI_NONAME:
      return CHIF_NET_RESLUT_NO_NAME;

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
    case EAI_FAMILY:
      return CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY;

    case ENOSPC:
      return CHIF_NET_RESULT_NOT_ENOUGH_SPACE;

    case EPIPE:
      return CHIF_NET_RESULT_CONNECTION_CLOSED;

    case EMSGSIZE:
      return CHIF_NET_RESULT_TOO_LONG_MSG_NOT_SENT;

    case ENOBUFS:
    case ENOMEM:
    case EAI_MEMORY:
      return CHIF_NET_RESULT_NO_MEMORY;

    case EPROTONOSUPPORT:
    case EAI_SERVICE:
      return CHIF_NET_RESULT_INVALID_TRANSPORT_PROTOCOL;

    case EAI_SOCKTYPE:
      return CHIF_NET_RESULT_INVALID_SOCKTYPE;

    case EPERM:
      return CHIF_NET_RESULT_ACCESS_DENIED;

    case ENETUNREACH:
      return CHIF_NET_RESULT_NO_NETWORK;

    case EAI_FAIL:
    case EAI_AGAIN:
      return CHIF_NET_RESULT_NAME_SERVER_FAIL;

    case EAI_NONAME:
      return CHIF_NET_RESLUT_NO_NAME;

    case EFAULT:
      return CHIF_NET_RESULT_BUFFER_BAD;

    case EAI_BADFLAGS:
      return CHIF_NET_RESULT_INVALID_INPUT_PARAM;

    default:
      return CHIF_NET_RESULT_UNKNOWN;
  }
#else
  return CHIF_NET_RESULT_UNKNOWN;
#endif
}

static chif_net_result
_chif_net_ai_error_to_result(const int result)
{
  switch (result) {

    case EAI_MEMORY:
      return CHIF_NET_RESULT_NO_MEMORY;

    case EAI_SERVICE:
      return CHIF_NET_RESULT_INVALID_TRANSPORT_PROTOCOL;

    case EAI_SOCKTYPE:
      return CHIF_NET_RESULT_INVALID_SOCKTYPE;

    case EAI_FAIL:
    case EAI_AGAIN:
      return CHIF_NET_RESULT_NAME_SERVER_FAIL;

    case EAI_NONAME:
      return CHIF_NET_RESLUT_NO_NAME;

    case EAI_BADFLAGS:
      return CHIF_NET_RESULT_INVALID_INPUT_PARAM;

    case EAI_FAMILY:
      return CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY;

    default:
      return CHIF_NET_RESULT_UNKNOWN;
  }
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
static chif_net_result
_chif_net_setsockopt(chif_net_socket socket,
                     int level,
                     int optname,
                     const void* optval,
                     socklen_t optlen)
{
  int res_not_ok;

#if defined(CHIF_NET_BERKLEY_SOCKET) || defined(CHIF_NET_WINSOCK2)
  res_not_ok = setsockopt(socket, level, optname, optval, optlen);
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
 * @param socket Socket to check.
 * @param res_out Output parameter, 0 for fail, or 1 for success.
 * @param events Check man page for poll for the possible events.
 * @param timeout_ms If event being polled has not happened, how long before
 *                   we should timeout and return? Use 0 to return instantly.
 * @return The result of the syscall translated into chif_net_result types.
 */
static chif_net_result
_chif_net_poll(const chif_net_socket socket,
               int* res_out,
               const short events,
               const int timeout_ms)
{
  chif_net_check check;
  check.socket = socket;
  check.request_events = events;
  check.return_events = 0;
  const int check_count = 1;

  chif_net_result res = chif_net_poll(&check, check_count, res_out, timeout_ms);

  // TODO
  if (res != CHIF_NET_RESULT_SUCCESS) {
    return res;
  }
  if (*res_out > 0) {
    if (check.return_events & events) {
      *res_out = 1;
    } else {
      *res_out = 0;
      if (check.return_events & POLLERR) {
        return CHIF_NET_RESULT_FAIL;
      } else if (check.return_events & POLLHUP) {
        return CHIF_NET_RESULT_CONNECTION_CLOSED;
      } else if (check.return_events & POLLNVAL) {
        return CHIF_NET_RESULT_INVALID_FILE_DESCRIPTOR;
      }
    }
  }

  return CHIF_NET_RESULT_SUCCESS;
}

static socklen_t
_chif_net_address_size_from_address_family(
  const chif_net_address_family address_family)
{
  socklen_t addrlen;
  if (address_family == CHIF_NET_ADDRESS_FAMILY_IPV4) {
    addrlen = sizeof(struct sockaddr_in);
  } else if (address_family == CHIF_NET_ADDRESS_FAMILY_IPV6) {
    addrlen = sizeof(struct sockaddr_in6);
  } else {
    return CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY;
  }
  return addrlen;
}

// ====================================================================== //
// Implementation
// ====================================================================== //

chif_net_result
chif_net_startup()
{
#if defined(CHIF_NET_WINSOCK2)
  WSADATA winsock_data;
  const int result = WSAStartup(WINSOCK_VERSION, &winsock_data);
  if (result != 0) {
    return CHIF_NET_RESULT_FAIL;
  }
#endif
  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_shutdown()
{
#if defined(CHIF_NET_WINSOCK2)
  const int32_t result = WSACleanup();
  if (result == WSAEINPROGRESS) {
    return CHIF_NET_RESULT_BLOCKING;
  } else if (result != 0) {
    return CHIF_NET_RESULT_FAIL;
  }
#endif
  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_open_socket(chif_net_socket* socket_out,
                     const chif_net_transport_protocol transport_protocol,
                     const chif_net_address_family address_family)
{
  const int domain = address_family;
  const int protocol = transport_protocol;

  int type;
  switch (transport_protocol) {
    case CHIF_NET_TRANSPORT_PROTOCOL_TCP: {
      type = SOCK_STREAM;
      break;
    }
    case CHIF_NET_TRANSPORT_PROTOCOL_UDP: {
      type = SOCK_DGRAM;
      break;
    }
    default:
      return CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY;
  }

  *socket_out = socket(domain, type, protocol);

  if (*socket_out == CHIF_NET_INVALID_SOCKET) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_close_socket(chif_net_socket* socket_out)
{
  if (*socket_out != CHIF_NET_INVALID_SOCKET) {
    // Close the socket
#if defined(CHIF_NET_WINSOCK2)
    const int result = closesocket(*socket_out);
#elif defined(CHIF_NET_BERKLEY_SOCKET)
    const int result = close(*socket_out);
#else
    return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif

    // Set socket to invalid to prevent usage of closed socket.
    *socket_out = CHIF_NET_INVALID_SOCKET;

    if (result == -1) {
      return _chif_net_get_specific_result_type();
    }
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_connect(const chif_net_socket socket, const chif_net_address* address)
{
  int result;
  if (address->address_family == CHIF_NET_ADDRESS_FAMILY_IPV4) {
    struct sockaddr_in addr;
    memcpy(&addr, address, sizeof(chif_net_ipv4_address));
    memset(((uint8_t*)&addr) + sizeof(chif_net_ipv4_address),
           0,
           sizeof(struct sockaddr_in) - sizeof(chif_net_ipv4_address));
    result = connect(socket, (const struct sockaddr*)&addr, sizeof(addr));
  } else if (address->address_family == CHIF_NET_ADDRESS_FAMILY_IPV6) {
    result = connect(
      socket, (const struct sockaddr*)address, sizeof(chif_net_ipv6_address));
  } else {
    return CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY;
  }

  if (result == -1) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_bind(const chif_net_socket socket, const chif_net_address* address)
{
  int result;
  if (address->address_family == CHIF_NET_ADDRESS_FAMILY_IPV4) {
    struct sockaddr_in addr;
    memcpy(&addr, address, sizeof(chif_net_ipv4_address));
    memset(((uint8_t*)&addr) + sizeof(chif_net_ipv4_address),
           0,
           sizeof(struct sockaddr_in) - sizeof(chif_net_ipv4_address));
    result = bind(socket, (const struct sockaddr*)&addr, sizeof(addr));
  } else if (address->address_family == CHIF_NET_ADDRESS_FAMILY_IPV6) {
    result = bind(
      socket, (const struct sockaddr*)address, sizeof(chif_net_ipv6_address));
  } else {
    return CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY;
  }

  if (result != 0) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_listen(const chif_net_socket socket, const int maximum_backlog)
{
  const int result = listen(socket, maximum_backlog);

  if (result != 0) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_accept(const chif_net_socket listening_socket,
                chif_net_address* client_address_out,
                chif_net_socket* client_socket_out)
{
  socklen_t client_addrlen = _chif_net_address_size_from_address_family(
    client_address_out->address_family);
  const socklen_t addrlen_copy = client_addrlen;

  if (client_address_out->address_family == CHIF_NET_ADDRESS_FAMILY_IPV4) {
    struct sockaddr_in addr;
    *client_socket_out =
      accept(listening_socket, (struct sockaddr*)&addr, &client_addrlen);
    memcpy(client_address_out, &addr, sizeof(chif_net_ipv4_address));
  } else if (client_address_out->address_family ==
             CHIF_NET_ADDRESS_FAMILY_IPV6) {
    *client_socket_out = accept(
      listening_socket, (struct sockaddr*)client_address_out, &client_addrlen);
  } else {
    return CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY;
  }

  if (client_addrlen > addrlen_copy) {
    return CHIF_NET_RESULT_BUFSIZE_INVALID;
  }

  /* TODO
     Linux  accept() (and accept4()) passes already-pending network errors on
     the new socket as an error code from accept().  This behavior differs from
       other BSD socket implementations.  For reliable operation the application
     should detect the network errors defined for the protocol after  accept()
       and  treat  them like EAGAIN by retrying.  In the case of TCP/IP, these
     are ENETDOWN, EPROTO, ENOPROTOOPT, EHOSTDOWN, ENONET, EHOSTUNREACH, EOPNOT‐
       SUPP, and ENETUNREACH.
   */

  if (*client_socket_out == -1) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_read(const chif_net_socket socket,
              uint8_t* buf_out,
              const size_t bufsize,
              int* read_bytes_out)
{
  if (socket == CHIF_NET_INVALID_SOCKET) {
    return CHIF_NET_RESULT_NOT_A_SOCKET;
  }

  int flag = 0;
#if defined(CHIF_NET_BERKLEY_SOCKET)
  // Prevent SIGPIPE signal and handle the error in application code
  flag = MSG_NOSIGNAL;
#endif

#if defined(CHIF_NET_WINSOCK2)
  if (bufsize > INT_MAX) {
    return CHIF_NET_RESULT_BUFSIZE_INVALID;
  }
  const int result = recv(socket, (char*)buf_out, (int)bufsize, flag);
#else
  const int result = recv(socket, buf_out, bufsize, flag);
#endif

  if (result == -1) {
    return _chif_net_get_specific_result_type();
  }
  // TODO result == 0 may indicate connection closed if TCP
  /* else if (result == 0) */
  /*   return CHIF_NET_RESULT_CONNECTION_CLOSED; */

  if (read_bytes_out) {
    *read_bytes_out = result;
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_readfrom(const chif_net_socket socket,
                  uint8_t* buf_out,
                  const size_t bufsize,
                  int* read_bytes_out,
                  chif_net_address* from_address_out)
{
  if (socket == CHIF_NET_INVALID_SOCKET) {
    return CHIF_NET_RESULT_NOT_A_SOCKET;
  }

  int flag = 0;
#if defined(CHIF_NET_BERKLEY_SOCKET)
  // Prevent SIGPIPE signal and handle the error in application code
  flag = MSG_NOSIGNAL;
#endif

  socklen_t addrlen = _chif_net_address_size_from_address_family(
    from_address_out->address_family);
  const socklen_t addrlen_copy = addrlen;

#if defined(CHIF_NET_WINSOCK2)
  if (bufsize > INT_MAX) {
    return CHIF_NET_RESULT_BUFSIZE_INVALID;
  }
#endif

  int result;
  if (from_address_out->address_family == CHIF_NET_ADDRESS_FAMILY_IPV4) {
    struct sockaddr_in addr;
#if defined(CHIF_NET_WINSOCK2)
    result = recvfrom(socket,
                      (char*)buf_out,
                      (int)bufsize,
                      flag,
                      (struct sockaddr*)&addr,
                      &addrlen);
#else
    result = recvfrom(
      socket, buf_out, bufsize, flag, (struct sockaddr*)&addr, &addrlen);
#endif
    memcpy(from_address_out, &addr, sizeof(chif_net_ipv4_address));
  } else if (from_address_out->address_family == CHIF_NET_ADDRESS_FAMILY_IPV6) {
#if defined(CHIF_NET_WINSOCK2)
    result = recvfrom(socket,
                      (char*)buf_out,
                      (int)bufsize,
                      flag,
                      (struct sockaddr*)from_address_out,
                      &addrlen);
#else
    result = recvfrom(socket,
                      buf_out,
                      bufsize,
                      flag,
                      (struct sockaddr*)from_address_out,
                      &addrlen);
#endif
  } else {
    return CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY;
  }

  if (result == -1) {
    return _chif_net_get_specific_result_type();
  }
  // TODO result == 0 may indicate connection closed if TCP
  /* else if (!result) { */
  /*   return CHIF_NET_RESULT_CONNECTION_CLOSED; */
  /* } */

  if (addrlen > addrlen_copy) {
    return CHIF_NET_RESULT_BUFSIZE_INVALID;
  }

  if (read_bytes_out) {
    *read_bytes_out = result;
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_write(const chif_net_socket socket,
               const uint8_t* buf,
               const size_t bufsize,
               int* sent_bytes_out)
{
  if (socket == CHIF_NET_INVALID_SOCKET) {
    return CHIF_NET_RESULT_NOT_A_SOCKET;
  }

  int flag = 0;
#if defined(CHIF_NET_BERKLEY_SOCKET)
  // Prevent SIGPIPE signal handle the error in application code
  flag = MSG_NOSIGNAL;
#endif

#if defined(CHIF_NET_WINSOCK2)
  if (bufsize > INT_MAX) {
    return CHIF_NET_RESULT_BUFSIZE_INVALID;
  }
  const int result = send(socket, (const char*)buf, (int)bufsize, flag);
#else
  const int result = send(socket, buf, bufsize, flag);
#endif

  if (result == -1) {
    return _chif_net_get_specific_result_type();
  }

  if (sent_bytes_out) {
    *sent_bytes_out = result;
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_writeto(const chif_net_socket socket,
                 const uint8_t* buf,
                 const size_t bufsize,
                 int* sent_bytes_out,
                 const chif_net_address* to_address)
{
  if (socket == CHIF_NET_INVALID_SOCKET) {
    return CHIF_NET_RESULT_NOT_A_SOCKET;
  }

  int flag = 0;
#if defined(CHIF_NET_BERKLEY_SOCKET)
  // Prevent SIGPIPE signal handle the error in application code
  flag = MSG_NOSIGNAL;
#endif

#if defined(CHIF_NET_WINSOCK2)
  if (bufsize > INT_MAX) {
    return CHIF_NET_RESULT_BUFSIZE_INVALID;
  }
#endif

  int result;
  if (to_address->address_family == CHIF_NET_ADDRESS_FAMILY_IPV4) {
    struct sockaddr_in addr;
    memcpy(&addr, to_address, sizeof(chif_net_ipv4_address));
    memset(((uint8_t*)&addr) + sizeof(chif_net_ipv4_address),
           0,
           sizeof(struct sockaddr_in) - sizeof(chif_net_ipv4_address));
#if defined(CHIF_NET_WINSOCK2)
    result = sendto(socket,
                    (const char*)buf,
                    (int)bufsize,
                    flag,
                    (const struct sockaddr*)&addr,
                    sizeof(struct sockaddr_in));
#else
    result = sendto(socket,
                    buf,
                    bufsize,
                    flag,
                    (const struct sockaddr*)&addr,
                    sizeof(struct sockaddr_in));
#endif
  } else if (to_address->address_family == CHIF_NET_ADDRESS_FAMILY_IPV6) {
#if defined(CHIF_NET_WINSOCK2)
    result = sendto(socket,
                    (const char*)buf,
                    (int)bufsize,
                    flag,
                    (const struct sockaddr*)to_address,
                    sizeof(struct sockaddr_in6));
#else
    result = sendto(socket,
                    buf,
                    bufsize,
                    flag,
                    (const struct sockaddr*)to_address,
                    sizeof(struct sockaddr_in6));
#endif
  } else {
    return CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY;
  }

  if (result == -1) {
    return _chif_net_get_specific_result_type();
  }

  if (sent_bytes_out) {
    *sent_bytes_out = result;
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_poll(chif_net_check* check,
              const size_t check_count,
              int* ready_count_out,
              const int timeout_ms)
{
#if defined(CHIF_NET_WINSOCK2)
  *ready_count_out = WSAPoll((struct pollfd*)check, (ULONG)check_count, timeout_ms);
#else
  *ready_count_out = poll((struct pollfd*)check, check_count, timeout_ms);
#endif

  if (*ready_count_out < 0) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_can_read(const chif_net_socket socket,
                  int* can_read_out,
                  const int timeout_ms)
{
  return _chif_net_poll(socket, can_read_out, POLLIN, timeout_ms);
}

chif_net_result
chif_net_can_write(const chif_net_socket socket,
                   int* can_write_out,
                   const int timeout_ms)
{
  return _chif_net_poll(socket, can_write_out, POLLOUT, timeout_ms);
}

chif_net_result
chif_net_has_error(const chif_net_socket socket, const int timeout_ms)
{
  int socket_no_error;
  const chif_net_result res =
    _chif_net_poll(socket, &socket_no_error, 0, timeout_ms);
  return res;
}

chif_net_result
chif_net_set_blocking(const chif_net_socket socket,
                      const chif_net_bool blocking)
{
#if defined(CHIF_NET_WINSOCK2)
  u_long blocking_mode = !blocking;
  int result = ioctlsocket(socket, (long)FIONBIO, &blocking_mode);
  if (result != 0) {
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

chif_net_result
chif_net_create_address(chif_net_address* address_out,
                        const char* name,
                        const char* service,
                        const chif_net_address_family address_family,
                        const chif_net_transport_protocol transport_protocol)
{
  struct addrinfo hints, *ai;
  memset(&hints, 0, sizeof(hints));

  hints.ai_family = address_family;
  hints.ai_protocol = transport_protocol;
  switch (transport_protocol) {
    case CHIF_NET_TRANSPORT_PROTOCOL_TCP:
      hints.ai_socktype = SOCK_STREAM;
      break;
    case CHIF_NET_TRANSPORT_PROTOCOL_UDP:
      // fall through
    default:
      hints.ai_socktype = SOCK_DGRAM;
  }

  if (name == NULL) {
    hints.ai_flags = AI_PASSIVE; // wildcard IP address
  }

  const int result = getaddrinfo(name, service, &hints, &ai);
  if (result != 0) {
    // no need to freeaddrinfo() here
    return _chif_net_ai_error_to_result(result);
  }

  switch (ai->ai_family) {
    case CHIF_NET_ADDRESS_FAMILY_IPV4: {
      memcpy(address_out, ai->ai_addr, sizeof(chif_net_ipv4_address));
      break;
    }
    case CHIF_NET_ADDRESS_FAMILY_IPV6: {
      memcpy(address_out, ai->ai_addr, sizeof(struct sockaddr_in6));
      break;
    }
    default:
      freeaddrinfo(ai);
      return CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY;
  }

  freeaddrinfo(ai);

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_address_from_socket(const chif_net_socket socket,
                             chif_net_address* address_out)
{
  socklen_t addrlen =
    _chif_net_address_size_from_address_family(address_out->address_family);
  const socklen_t addrlen_copy = addrlen;

  int result;
  if (address_out->address_family == CHIF_NET_ADDRESS_FAMILY_IPV4) {
    struct sockaddr_in addr;
    result = getsockname(socket, (struct sockaddr*)&addr, &addrlen);
    memcpy(address_out, &addr, sizeof(chif_net_ipv4_address));
  } else if (address_out->address_family == CHIF_NET_ADDRESS_FAMILY_IPV6) {
    result = getsockname(socket, (struct sockaddr*)address_out, &addrlen);
  } else {
    return CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY;
  }

  if (result != 0) {
    return _chif_net_get_specific_result_type();
  }

  if (addrlen > addrlen_copy) {
    return CHIF_NET_RESULT_BUFSIZE_INVALID;
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_peer_address_from_socket(const chif_net_socket socket,
                                  chif_net_address* peer_address_out)
{
  socklen_t addrlen = _chif_net_address_size_from_address_family(
    peer_address_out->address_family);
  const socklen_t addrlen_copy = addrlen;

  int result;
  if (peer_address_out->address_family == CHIF_NET_ADDRESS_FAMILY_IPV4) {
    struct sockaddr_in addr;
    result = getpeername(socket, (struct sockaddr*)&addr, &addrlen);
    memcpy(peer_address_out, &addr, sizeof(chif_net_ipv4_address));
  } else if (peer_address_out->address_family == CHIF_NET_ADDRESS_FAMILY_IPV6) {
    result = getpeername(socket, (struct sockaddr*)peer_address_out, &addrlen);
  } else {
    return CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY;
  }

  if (result != 0) {
    return _chif_net_get_specific_result_type();
  }

  if (addrlen > addrlen_copy) {
    return CHIF_NET_RESULT_BUFSIZE_INVALID;
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_ip_from_socket(const chif_net_socket socket,
                        char* str_out,
                        const size_t strlen)
{
  chif_net_address address;
  address.address_family = CHIF_NET_ADDRESS_FAMILY_IPV6;

  const chif_net_result result =
    chif_net_address_from_socket(socket, (chif_net_address*)&address);
  if (result != CHIF_NET_RESULT_SUCCESS) {
    return result;
  }

  return chif_net_ip_from_address((chif_net_address*)&address, str_out, strlen);
}

chif_net_result
chif_net_ip_from_address(const chif_net_address* address,
                         char* str_out,
                         const size_t strlen)
{
  const char* ntop_result;
  if (address->address_family == CHIF_NET_ADDRESS_FAMILY_IPV4) {
    ntop_result = inet_ntop(address->address_family,
                            &((const chif_net_ipv4_address*)address)->address,
                            str_out,
                            strlen);
  } else /* if (address->address_family == CHIF_NET_ADDRESS_FAMILY_IPV6) */ {
    ntop_result = inet_ntop(address->address_family,
                            &((const chif_net_ipv6_address*)address)->address,
                            str_out,
                            strlen);
  }

  if (ntop_result == NULL) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_port_from_socket(const chif_net_socket socket, chif_net_port* port_out)
{
  chif_net_address address;
  address.address_family = CHIF_NET_ADDRESS_FAMILY_IPV6;

  const chif_net_result result =
    chif_net_address_from_socket(socket, (chif_net_address*)&address);

  if (result != CHIF_NET_RESULT_SUCCESS) {
    return result;
  }

  return chif_net_port_from_address((chif_net_address*)&address, port_out);
}

chif_net_result
chif_net_port_from_address(const chif_net_address* address,
                           chif_net_port* port_out)
{

  if (address->address_family == CHIF_NET_ADDRESS_FAMILY_IPV4) {
    *port_out = ntohs(((const chif_net_ipv4_address*)address)->port);
  } else if (address->address_family == CHIF_NET_ADDRESS_FAMILY_IPV6) {
    *port_out = ntohs(((const chif_net_ipv6_address*)address)->port);
  } else {
    return CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY;
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_get_bytes_available(const chif_net_socket socket,
                             unsigned long* bytes_available_out)
{
#if defined(CHIF_NET_WINSOCK2)
  const int result = ioctlsocket(socket, FIONREAD, bytes_available_out);
#elif defined(CHIF_NET_BERKLEY_SOCKET)
  const int result = ioctl(socket, FIONREAD, bytes_available_out);
#else
  return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif

  if (result == -1) {
    return _chif_net_get_specific_result_type();
  }

  return CHIF_NET_RESULT_SUCCESS;
}

chif_net_result
chif_net_set_reuse_addr(const chif_net_socket socket, const chif_net_bool reuse)
{
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
}

chif_net_result
chif_net_set_reuse_port(const chif_net_socket socket, const chif_net_bool reuse)
{
#if defined(CHIF_NET_HAS_TCP_DETAILS)
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
#else
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(reuse);
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(socket);
  return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif
}

chif_net_result
chif_net_set_keepalive(const chif_net_socket socket,
                       const chif_net_bool keepalive)
{
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
}

chif_net_result
chif_net_set_broadcast(const chif_net_socket socket,
                       const chif_net_bool broadcast)
{
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
}

chif_net_result
chif_net_set_recv_timeout(const chif_net_socket socket, const int time_ms)
{
#if defined(CHIF_NET_BERKLEY_SOCKET)
  struct timeval timeout;
  timeout.tv_sec = time_ms / 1000;
  timeout.tv_usec = (time_ms % 1000) * 1000;
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#elif defined(CHIF_NET_WINSOCK2)
  const DWORD timeout = (DWORD)time_ms;
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#else
  return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif
}

chif_net_result
chif_net_set_send_timeout(const chif_net_socket socket, const int time_ms)
{
#if defined(CHIF_NET_BERKLEY_SOCKET)
  struct timeval timeout;
  timeout.tv_sec = time_ms / 1000;
  timeout.tv_usec = (time_ms % 1000) * 1000;
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#elif defined(CHIF_NET_WINSOCK2)
  const DWORD timeout = (DWORD)time_ms;
  return _chif_net_setsockopt(
    socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#else
  return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif
}

chif_net_result
chif_net_tcp_set_user_timeout(const chif_net_socket socket, const int time_ms)
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

chif_net_result
chif_net_tcp_set_nodelay(const chif_net_socket socket,
                         const chif_net_bool nodelay)
{
  return _chif_net_setsockopt(
    socket, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));
}

chif_net_result
chif_net_tcp_set_syncnt(const chif_net_socket socket, const int count)
{
#if defined(CHIF_NET_HAS_TCP_DETAILS)
  if (count < 0 || count > 255) {
    return CHIF_NET_RESULT_INVALID_INPUT_PARAM;
  }
  return _chif_net_setsockopt(
    socket, IPPROTO_TCP, TCP_SYNCNT, &count, sizeof(count));
#else
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(socket);
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(count);
  return CHIF_NET_RESULT_PLATFORM_NOT_SUPPORTED;
#endif
}

chif_net_result
chif_net_set_ttl(const chif_net_socket socket, const int ttl)
{
  return _chif_net_setsockopt(socket, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
}

chif_net_result
chif_net_set_own_iphdr(const chif_net_socket socket, const int provide_own_hdr)
{
  return _chif_net_setsockopt(
    socket, IPPROTO_IP, IP_HDRINCL, &provide_own_hdr, sizeof(provide_own_hdr));
}

// TODO remove?
/* chif_net_result */
/* chif_net_icmp_build(uint8_t* buf, */
/*                     size_t* bufsize, */
/*                     const void* data, */
/*                     size_t data_size, */
/*                     uint16_t id, */
/*                     uint16_t seq) */
/* { */
/* #if defined(CHIF_NET_BERKLEY_SOCKET) && defined(__linux__) */
/*   const size_t packet_size = */
/*     data_size + sizeof(struct iphdr) + sizeof(struct icmphdr); */
/*   if (*bufsize >= packet_size) { */
/*     struct icmphdr* icmp_hdr = (struct icmphdr*)(buf); */
/*     icmp_hdr->type = ICMP_ECHO; */
/*     icmp_hdr->code = 0; // MUST be 0 */
/*     icmp_hdr->un.echo.id = id; */
/*     icmp_hdr->un.echo.sequence = seq; */
/*     icmp_hdr->checksum = 0; // must be zero when calculating actual checksum
 */
/*     memcpy(buf + sizeof(struct icmphdr), data, data_size); */
/*     icmp_hdr->checksum = _chif_net_0s_checksum(buf, packet_size); */

/*     *bufsize = packet_size; */
/*     return CHIF_NET_RESULT_SUCCESS; */
/*   } else { */
/*     return CHIF_NET_RESULT_NOT_ENOUGH_SPACE; */
/*   } */
/* #else */
/*   (void)buf; */
/*   (void)bufsize; */
/*   (void)data; */
/*   (void)data_size; */
/*   (void)id; */
/*   (void)seq; */
/*   assert(false && "not implemented"); */
/*   return CHIF_NET_RESULT_FAIL; */
/* #endif */
/* } */

const char*
chif_net_result_to_string(const chif_net_result result)
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
    case CHIF_NET_RESULT_INVALID_ADDRESS_FAMILY:
      return "INVALID_ADDRESS_FAMILY";
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
    case CHIF_NET_RESULT_INVALID_TRANSPORT_PROTOCOL:
      return "INVALID_TRANSPORT_PROTOCOL";
    case CHIF_NET_RESULT_NO_MEMORY:
      return "NO_MEMORY";
    case CHIF_NET_RESULT_NO_NETWORK:
      return "NO_NETWORK";
    case CHIF_NET_RESULT_BLOCKING_CANCELED:
      return "BLOCKING_CANCELED";
    case CHIF_NET_RESULT_NET_UNREACHABLE:
      return "NET_UNREACHABLE";
    case CHIF_NET_RESULT_BUFSIZE_INVALID:
      return "BUFSIZE_INVALID";
    case CHIF_NET_RESULT_NAME_SERVER_FAIL:
      return "NAME_SERVER_FAIL";
    case CHIF_NET_RESLUT_NO_NAME:
      return "NO_NAME";
    case CHIF_NET_RESULT_BUFFER_BAD:
      return "BUFFER_BAD";
    case CHIF_NET_RESULT_INVALID_SOCKTYPE:
      return "INVALID_SOCKTYPE";
  }

  // should never happen
  return "INTERNAL_CHIF_ERROR";
}

const char*
chif_net_address_family_to_string(const chif_net_address_family address_family)
{
  switch (address_family) {
    case CHIF_NET_ADDRESS_FAMILY_IPV4: {
      return "IPv4";
    }
    case CHIF_NET_ADDRESS_FAMILY_IPV6: {
      return "IPv6";
    }
    default: {
      return "INVALID INPUT";
    }
  }
}

const char*
chif_net_transport_protocol_to_string(
  const chif_net_transport_protocol transport_protocol)
{
  switch (transport_protocol) {
    case CHIF_NET_TRANSPORT_PROTOCOL_TCP: {
      return "TCP";
    }
    case CHIF_NET_TRANSPORT_PROTOCOL_UDP: {
      return "UDP";
    }
    default: {
      return "INVALID INPUT";
    }
  }
}
