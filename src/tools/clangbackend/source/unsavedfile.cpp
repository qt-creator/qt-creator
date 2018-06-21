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

#include "unsavedfile.h"

#include "clangfilepath.h"
#include "utf8positionfromlinecolumn.h"

#include <ostream>

#include <utils/qtcassert.h>

namespace ClangBackEnd {

UnsavedFile::UnsavedFile()
{
}

UnsavedFile::UnsavedFile(const Utf8String &filePath,
                         const Utf8String &fileContent)
    : m_filePath(filePath)
    , m_nativeFilePath(FilePath::toNativeSeparators(filePath))
    , m_fileContent(fileContent)
{
}

Utf8String UnsavedFile::nativeFilePath() const
{
    return m_nativeFilePath;
}

Utf8String UnsavedFile::filePath() const
{
    return m_filePath;
}

Utf8String UnsavedFile::fileContent() const
{
    return m_fileContent;
}

uint UnsavedFile::toUtf8Position(uint line, uint column, bool *ok) const
{
    Utf8PositionFromLineColumn converter(m_fileContent.constData());
    if (converter.find(line, column)) {
        *ok = true;
        return converter.position();
    }

    *ok = false;
    return 0;
}

bool UnsavedFile::hasCharacterAt(uint line, uint column, char character) const
{
    bool positionIsOk = false;
    const uint utf8Position = toUtf8Position(line, column, &positionIsOk);

    return positionIsOk && hasCharacterAt(utf8Position, character);
}

Utf8String UnsavedFile::lineRange(uint fromLine, uint toLine) const
{
    QTC_ASSERT(fromLine <= toLine, return Utf8String());

    // Find start of first line
    bool ok = false;
    const uint fromPosition = toUtf8Position(fromLine, 1, &ok);
    QTC_ASSERT(ok, return Utf8String());

    // Find end of last line
    uint toPosition = toUtf8Position(toLine, 1, &ok);
    QTC_ASSERT(ok, return Utf8String());
    const uint endPosition = uint(m_fileContent.byteSize());
    while (toPosition < endPosition && m_fileContent.constData()[toPosition] != '\n')
        ++toPosition;

    return m_fileContent.mid(int(fromPosition), int(toPosition - fromPosition));
}

bool UnsavedFile::hasCharacterAt(uint position, char character) const
{
    if (position < uint(m_fileContent.byteSize()))
        return m_fileContent.constData()[position] == character;

    return false;
}

bool UnsavedFile::replaceAt(uint position, uint length, const Utf8String &replacement)
{
    if (position < uint(m_fileContent.byteSize())) {
        m_fileContent.replace(int(position), int(length), replacement);
        return true;
    }

    return false;
}

std::ostream &operator<<(std::ostream &os, const UnsavedFile &unsavedFile)
{
    os << "UnsavedFile("
       << unsavedFile.m_filePath << ", "
       << unsavedFile.m_fileContent << ", "
       << unsavedFile.m_fileContent << ")";

    return os;
}

} // namespace ClangBackEnd
