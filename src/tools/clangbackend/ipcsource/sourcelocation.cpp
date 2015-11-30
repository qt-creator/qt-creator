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
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "sourcelocation.h"

#include "clangstring.h"
#include "translationunit.h"

#include <utf8string.h>

#include <ostream>
#include <sourcelocationcontainer.h>

namespace ClangBackEnd {

SourceLocation::SourceLocation()
    : cxSourceLocation(clang_getNullLocation())
{
}

const Utf8String &SourceLocation::filePath() const
{
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
    return SourceLocationContainer(filePath_, line_, offset_);
}

SourceLocation::SourceLocation(CXSourceLocation cxSourceLocation)
    : cxSourceLocation(cxSourceLocation)
{
    CXFile cxFile;

    clang_getFileLocation(cxSourceLocation,
                          &cxFile,
                          &line_,
                          &column_,
                          &offset_);

    filePath_ = ClangString(clang_getFileName(cxFile));
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
      filePath_(filePath),
      line_(line)
{
}

bool operator==(const SourceLocation &first, const SourceLocation &second)
{
    return clang_equalLocations(first.cxSourceLocation, second.cxSourceLocation);
}

SourceLocation::operator CXSourceLocation() const
{
    return cxSourceLocation;
}

void PrintTo(const SourceLocation &sourceLocation, std::ostream *os)
{
    auto filePath = sourceLocation.filePath();
    if (filePath.hasContent())
        *os << filePath.constData()  << ", ";

    *os << "line: " << sourceLocation.line()
        << ", column: "<< sourceLocation.column()
        << ", offset: "<< sourceLocation.offset();
}

} // namespace ClangBackEnd

