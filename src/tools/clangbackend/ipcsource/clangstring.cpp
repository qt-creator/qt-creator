/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangstring.h"

#include <memory>

namespace ClangBackEnd {

ClangString::ClangString(CXString cxString)
    : cxString(cxString)
{
}

ClangString::~ClangString()
{
    clang_disposeString(cxString);
}

bool ClangString::isNull() const
{
    return cxString.data == nullptr;
}

ClangString &ClangString::operator=(ClangString &&other)
{
    if (this != &other) {
        clang_disposeString(cxString);
        cxString = std::move(other.cxString);
        other.cxString.data = nullptr;
        other.cxString.private_flags = 0;
    }

    return *this;
}

const char *ClangString::cString() const
{
    return clang_getCString(cxString);
}

ClangString::ClangString(ClangString &&other)
    : cxString(std::move(other.cxString))
{
    other.cxString.data = nullptr;
    other.cxString.private_flags = 0;
}

ClangString::operator Utf8String() const
{
    return Utf8String(cString(), -1);
}

} // namespace ClangBackEnd

