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

#include "sourcelocation.h"

#include "clangdocument.h"
#include "clangfilepath.h"
#include "clangstring.h"

#include <clangsupport/sourcelocationcontainer.h>

#include <sqlite/utf8string.h>

#include <utils/textutils.h>

#include <ostream>

namespace ClangBackEnd {

SourceLocation::SourceLocation()
    : cxSourceLocation(clang_getNullLocation())
{
}

const Utf8String &SourceLocation::filePath() const
{
    if (isFilePathNormalized_)
        return filePath_;

    isFilePathNormalized_ = true;
    filePath_ = FilePath::fromNativeSeparators(filePath_);

    return filePath_;
}

uint SourceLocation::line() const
{
    return line_;
}

uint SourceLocation::column() const
{
    return column_;
}

uint SourceLocation::offset() const
{
    return offset_;
}

SourceLocationContainer SourceLocation::toSourceLocationContainer() const
{
    return SourceLocationContainer(filePath(), line_, column_);
}

SourceLocation::SourceLocation(CXTranslationUnit cxTranslationUnit,
                               CXSourceLocation cxSourceLocation)
    : cxSourceLocation(cxSourceLocation)
    , cxTranslationUnit(cxTranslationUnit)
{
    CXFile cxFile;

    clang_getFileLocation(cxSourceLocation,
                          &cxFile,
                          &line_,
                          &column_,
                          &offset_);

    isFilePathNormalized_ = false;
    if (!cxFile)
        return;

    filePath_ = ClangString(clang_getFileName(cxFile));
    if (column_ > 1) {
        const uint lineStart = offset_ + 1 - column_;
        const char *contents = clang_getFileContents(cxTranslationUnit, cxFile, nullptr);
        if (!contents)
            return;
        // (1) column in SourceLocation is the actual column shown by CppEditor.
        // (2) column in Clang is the utf8 byte offset from the beginning of the line.
        // Here we convert column from (2) to (1).
        column_ = static_cast<uint>(QString::fromUtf8(&contents[lineStart],
                                                      static_cast<int>(column_)).size());
    }
}

SourceLocation::SourceLocation(CXTranslationUnit cxTranslationUnit,
                               const Utf8String &filePath,
                               uint line,
                               uint column)
    : cxSourceLocation(clang_getLocation(cxTranslationUnit,
                                         clang_getFile(cxTranslationUnit,
                                                       filePath.constData()),
                                         line,
                                         column)),
      cxTranslationUnit(cxTranslationUnit),
      filePath_(filePath),
      line_(line),
      column_(column),
      isFilePathNormalized_(true)
{
    clang_getFileLocation(cxSourceLocation, 0, 0, 0, &offset_);
}

SourceLocation::operator CXSourceLocation() const
{
    return cxSourceLocation;
}

std::ostream &operator<<(std::ostream &os, const SourceLocation &sourceLocation)
{
    auto filePath = sourceLocation.filePath();
    if (filePath.hasContent())
        os << filePath  << ", ";

    os << "line: " << sourceLocation.line()
       << ", column: "<< sourceLocation.column()
       << ", offset: "<< sourceLocation.offset();

    return os;
}

} // namespace ClangBackEnd

