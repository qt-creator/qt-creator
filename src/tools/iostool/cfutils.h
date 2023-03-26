// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

#include <CoreFoundation/CoreFoundation.h>

namespace Ios {

template<typename CFType>
struct CFRefDeleter
{
    using pointer = CFType;
    void operator()(CFType ref) { CFRelease(ref); }
};

inline QString toQStringRelease(CFStringRef str)
{
    QString result = QString::fromCFString(str);
    CFRelease(str);
    return result;
}

using CFString_t = std::unique_ptr<CFStringRef, CFRefDeleter<CFStringRef>>;
using CFUrl_t = std::unique_ptr<CFURLRef, CFRefDeleter<CFURLRef>>;
using CFPropertyList_t = std::unique_ptr<CFPropertyListRef, CFRefDeleter<CFPropertyListRef>>;
using CFBundle_t = std::unique_ptr<CFBundleRef, CFRefDeleter<CFBundleRef>>;
using CFDictionary_t = std::unique_ptr<CFDictionaryRef, CFRefDeleter<CFDictionaryRef>>;
using CFArray_t = std::unique_ptr<CFArrayRef, CFRefDeleter<CFArrayRef>>;

} // namespace Ios
