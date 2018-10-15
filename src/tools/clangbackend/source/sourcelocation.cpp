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
    : m_cxSourceLocation(clang_getNullLocation())
{
}

const Utf8String &SourceLocation::filePath() const
{
    if (!m_isEvaluated)
        evaluate();

    if (m_isFilePathNormalized)
        return m_filePath;

    m_isFilePathNormalized = true;
    m_filePath = FilePath::fromNativeSeparators(m_filePath);

    return m_filePath;
}

uint SourceLocation::line() const
{
    if (!m_isEvaluated)
        evaluate();
    return m_line;
}

uint SourceLocation::column() const
{
    if (!m_isEvaluated)
        evaluate();
    return m_column;
}

uint SourceLocation::offset() const
{
    if (!m_isEvaluated)
        evaluate();
    return m_offset;
}

SourceLocationContainer SourceLocation::toSourceLocationContainer() const
{
    if (!m_isEvaluated)
        evaluate();
    return SourceLocationContainer(filePath(), m_line, m_column);
}

void SourceLocation::evaluate() const
{
    m_isEvaluated = true;

    CXFile cxFile;

    clang_getFileLocation(m_cxSourceLocation,
                          &cxFile,
                          &m_line,
                          &m_column,
                          &m_offset);

    m_isFilePathNormalized = false;
    if (!cxFile)
        return;

    m_filePath = ClangString(clang_getFileName(cxFile));
    if (m_column > 1) {
        const uint lineStart = m_offset + 1 - m_column;
        const char *contents = clang_getFileContents(m_cxTranslationUnit, cxFile, nullptr);
        if (!contents)
            return;
        // (1) column in SourceLocation is the actual column shown by CppEditor.
        // (2) column in Clang is the utf8 byte offset from the beginning of the line.
        // Here we convert column from (2) to (1).
        m_column = static_cast<uint>(QString::fromUtf8(&contents[lineStart],
                                                       static_cast<int>(m_column) - 1).size()) + 1;
    }
}

SourceLocation::SourceLocation(CXTranslationUnit cxTranslationUnit,
                               CXSourceLocation cxSourceLocation)
    : m_cxSourceLocation(cxSourceLocation)
    , m_cxTranslationUnit(cxTranslationUnit)
{
}

SourceLocation::operator CXSourceLocation() const
{
    return m_cxSourceLocation;
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

