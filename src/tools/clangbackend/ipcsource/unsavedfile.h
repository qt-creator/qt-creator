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

#include <clang-c/Index.h>

#include <iosfwd>

class Utf8String;

namespace ClangBackEnd {

using uint = unsigned int;

class UnsavedFile
{
public:
    friend void PrintTo(const UnsavedFile &unsavedFile, std::ostream *os);

    UnsavedFile(const Utf8String &filePath, const Utf8String &fileContent);
    ~UnsavedFile();

    UnsavedFile(const UnsavedFile &other) = delete;
    bool operator=(const UnsavedFile &other) = delete;

    UnsavedFile(UnsavedFile &&other) noexcept;
    UnsavedFile &operator=(UnsavedFile &&other) noexcept;

    const char *filePath() const;

    bool hasCharacterAt(uint position, char character) const;
    bool replaceAt(uint position, uint length, const Utf8String &replacement);

    CXUnsavedFile *data();

public: // for tests
    CXUnsavedFile cxUnsavedFile = { nullptr, nullptr, 0UL };
};

} // namespace ClangBackEnd
