/*
 * Copyright (c) 2004, Apple Computer, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of its
 *     contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "mdnsderived.h"
#include "cstddef"
#include "cstring"
#ifdef _WIN32
#define strncasecmp _strnicmp
#endif

namespace ZeroConf {
namespace Internal {

// DomainEndsInDot returns 1 if name ends with a dot, 0 otherwise
// (DNSServiceConstructFullName depends this returning 1 for true, rather than any non-zero value meaning true)
static int DomainEndsInDot(const char *dom)
{
    while (dom[0] && dom[1]) {
        if (dom[0] == '\\') { // advance past escaped byte sequence
            if ('0' <= dom[1] && dom[1] <= '9' &&
                '0' <= dom[2] && dom[2] <= '9' &&
                '0' <= dom[3] && dom[3] <= '9')
            {
                dom += 4;            // If "\ddd"    then skip four
            } else {
                dom += 2;            // else if "\x" then skip two
            }
        } else {
            dom++;                    // else goto next character
        }
    }
    return (dom[0] == '.');
}

// Note: Need to make sure we don't write more than kDNSServiceMaxDomainName (1009) bytes to fullName
// In earlier builds this constant was defined to be 1005, so to avoid buffer overruns on clients
// compiled with that constant we'll actually limit the output to 1005 bytes.
DNSServiceErrorType myDNSServiceConstructFullName(char       *const fullName,
                                                  const char *const service,      // May be NULL
                                                  const char *const regtype,
                                                  const char *const domain)
{
    const size_t len = !regtype ? 0 : strlen(regtype) - DomainEndsInDot(regtype);
    char       *fn   = fullName;
    char *const lim  = fullName + 1005;
    const char *s    = service;
    const char *r    = regtype;
    const char *d    = domain;

    // regtype must be at least "x._udp" or "x._tcp"
    if (len < 6 || !domain || !domain[0]) return kDNSServiceErr_BadParam;
    if (strncasecmp((regtype + len - 4), "_tcp", 4) && strncasecmp((regtype + len - 4), "_udp", 4)) return kDNSServiceErr_BadParam;

    if (service && *service)
    {
        while (*s)
        {
            unsigned char c = *s++;                // Needs to be unsigned, or values like 0xFF will be interpreted as < 32
            if (c <= ' ')                        // Escape non-printable characters
            {
                if (fn + 4 >= lim) goto fail;
                *fn++ = '\\';
                *fn++ = '0' + (c / 100);
                *fn++ = '0' + (c /  10) % 10;
                c     = '0' + (c      ) % 10;
            }
            else if (c == '.' || (c == '\\'))    // Escape dot and backslash literals
            {
                if (fn + 2 >= lim) goto fail;
                *fn++ = '\\';
            }
            else
                if (fn + 1 >= lim) goto fail;
            *fn++ = (char)c;
        }
        *fn++ = '.';
    }

    while (*r) if (fn + 1 >= lim) goto fail; else *fn++ = *r++;
    if (!DomainEndsInDot(regtype)) { if (fn + 1 >= lim) goto fail; else *fn++ = '.'; }

    while (*d) if (fn + 1 >= lim) goto fail; else *fn++ = *d++;
    if (!DomainEndsInDot(domain)) { if (fn + 1 >= lim) goto fail; else *fn++ = '.'; }

    *fn = '\0';
    return kDNSServiceErr_NoError;

fail:
    *fn = '\0';
    return kDNSServiceErr_BadParam;
}

uint16_t txtRecordGetCount(uint16_t txtLen, const void *txtRecord)
{
    uint16_t count = 0;
    uint8_t *p = (uint8_t*)txtRecord;
    uint8_t *e = p + txtLen;
    while (p<e) { p += 1 + p[0]; count++; }
    return((p>e) ? (uint16_t)0 : count);
}

DNSServiceErrorType txtRecordGetItemAtIndex(uint16_t txtLen, const void *txtRecord,
        uint16_t itemIndex, uint16_t keyBufLen, char *key, uint8_t *valueLen, const void **value)
{
    uint16_t count = 0;
    uint8_t *p = (uint8_t*)txtRecord;
    uint8_t *e = p + txtLen;
    while (p<e && count<itemIndex) { p += 1 + p[0]; count++; }    // Find requested item
    if (p<e && p + 1 + p[0] <= e) {    // If valid
        uint8_t *x = p+1;
        unsigned long len = 0;
        e = p + 1 + p[0];
        while (x+len<e && x[len] != '=') len++;
        if (len >= keyBufLen) return(kDNSServiceErr_NoMemory);
        memcpy(key, x, len);
        key[len] = 0;
        if (x+len<e) { // If we found '='
            *value = x + len + 1;
            *valueLen = (uint8_t)(p[0] - (len + 1));
        } else {
            *value = NULL;
            *valueLen = 0;
        }
        return(kDNSServiceErr_NoError);
    }
    return(kDNSServiceErr_Invalid);
}


} // namespace ZeroConf
} // namespace Internal
