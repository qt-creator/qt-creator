// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFURL.h>

#include <mach/error.h>

extern "C" {
struct service_conn_t {
    char unknown[0x10];
    int sockfd;
    void *sslContext = nullptr;
};
typedef service_conn_t *ServiceConnRef;
typedef unsigned int ServiceSocket; // match_port_t (i.e. natural_t) or socket (on windows, i.e sock_t)
} // extern C
