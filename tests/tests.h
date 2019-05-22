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

#ifndef TEST_H_
#define TEST_H_

#include <alf_test.h>

// ============================================================ //
// connect
// ============================================================ //
void
duckduckgo(AlfTestState* state);
void
bad_site(AlfTestState* state);

// ============================================================ //
// tcp
// ============================================================ //
void
tcp_test(AlfTestState* state);

// ============================================================ //
// poll
// ============================================================ //
void
poll_test(AlfTestState* state);

// ============================================================ //
// echo
// ============================================================ //
void
tcp_ipv4(AlfTestState* state);
void
tcp_ipv6(AlfTestState* state);
void
udp_ipv4(AlfTestState* state);
void
udp_ipv6(AlfTestState* state);

#endif // TEST_H_
