/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <clang-c/CXString.h>

#include <utf8string.h>

#include <cstring>

namespace ClangBackEnd {

class ClangString
{
public:
    ClangString(CXString cxString)
        : cxString(cxString)
    {
    }

    ~ClangString()
    {
        clang_disposeString(cxString);
    }

    ClangString(const ClangString &) = delete;
    const ClangString &operator=(const ClangString &) = delete;


    ClangString(ClangString &&other)
        : cxString(std::move(other.cxString))
    {
        other.cxString.data = nullptr;
        other.cxString.private_flags = 0;
    }


    ClangString &operator=(ClangString &&other)
    {
        if (this != &other) {
            clang_disposeString(cxString);
            cxString = std::move(other.cxString);
            other.cxString.data = nullptr;
            other.cxString.private_flags = 0;
        }

        return *this;
    }

    const char *cString() const
    {
        return clang_getCString(cxString);
    }

    operator Utf8String() const
    {
        return Utf8String(cString(), -1);
    }

    bool isNull() const
    {
        return cxString.data == nullptr;
    }



private:
    CXString cxString;
};

} // namespace ClangBackEnd
