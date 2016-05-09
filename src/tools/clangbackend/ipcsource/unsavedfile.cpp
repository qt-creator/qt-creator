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

#include "utf8string.h"
#include "utf8positionfromlinecolumn.h"

#include <cstring>
#include <ostream>

namespace ClangBackEnd {

UnsavedFile::UnsavedFile()
    : cxUnsavedFile(CXUnsavedFile{nullptr, nullptr, 0UL})
{
}

UnsavedFile::UnsavedFile(const Utf8String &filePath, const Utf8String &fileContent)
{
    char *cxUnsavedFilePath = new char[filePath.byteSize() + 1];
    char *cxUnsavedFileContent = new char[fileContent.byteSize() + 1];

    std::memcpy(cxUnsavedFilePath, filePath.constData(), filePath.byteSize() + 1);
    std::memcpy(cxUnsavedFileContent, fileContent.constData(), fileContent.byteSize() + 1);

    cxUnsavedFile = CXUnsavedFile{cxUnsavedFilePath,
                                  cxUnsavedFileContent,
                                  ulong(fileContent.byteSize())};
}

UnsavedFile::UnsavedFile(UnsavedFile &&other) Q_DECL_NOEXCEPT
    : cxUnsavedFile(other.cxUnsavedFile)
{
    other.cxUnsavedFile = { nullptr, nullptr, 0UL };
}

UnsavedFile &UnsavedFile::operator=(UnsavedFile &&other) Q_DECL_NOEXCEPT
{
    using std::swap;

    swap(this->cxUnsavedFile, other.cxUnsavedFile);

    return *this;
}

const char *UnsavedFile::filePath() const
{
    return cxUnsavedFile.Filename;
}

bool UnsavedFile::hasCharacterAt(uint line, uint column, char character) const
{
    Utf8PositionFromLineColumn converter(cxUnsavedFile.Contents);
    if (converter.find(line, column)) {
        const uint utf8Position = converter.position();
        return hasCharacterAt(utf8Position, character);
    }

    return false;
}

bool UnsavedFile::hasCharacterAt(uint position, char character) const
{
    if (position < cxUnsavedFile.Length)
        return cxUnsavedFile.Contents[position] == character;

    return false;
}

bool UnsavedFile::replaceAt(uint position, uint length, const Utf8String &replacement)
{
    if (position < cxUnsavedFile.Length) {
        Utf8String modifiedContent(cxUnsavedFile.Contents, cxUnsavedFile.Length);
        modifiedContent.replace(int(position), int(length), replacement);

        *this = UnsavedFile(Utf8String::fromUtf8(filePath()), modifiedContent);

        return true;
    }

    return false;
}

CXUnsavedFile *UnsavedFile::data()
{
    return &cxUnsavedFile;
}

UnsavedFile::~UnsavedFile()
{
    delete [] cxUnsavedFile.Contents;
    delete [] cxUnsavedFile.Filename;
    cxUnsavedFile.Contents = nullptr;
    cxUnsavedFile.Filename = nullptr;
    cxUnsavedFile.Length = 0;
}

void PrintTo(const UnsavedFile &unsavedFile, std::ostream *os)
{
    *os << "UnsavedFile("
           << unsavedFile.cxUnsavedFile.Filename << ", "
           << unsavedFile.cxUnsavedFile.Contents << ", "
           << unsavedFile.cxUnsavedFile.Length << ")";
}

} // namespace ClangBackEnd
