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

#include <utf8string.h>

namespace ClangBackEnd {

class SourceLocationContainer;
class Document;

class SourceLocation
{
    friend class SourceRange;
    friend class TranslationUnit;

    friend bool operator==(const SourceLocation &first, const SourceLocation &second)
    {
        return clang_equalLocations(first.m_cxSourceLocation, second.m_cxSourceLocation);
    }
    friend bool operator!=(const SourceLocation &first, const SourceLocation &second)
    {
        return !(first == second);
    }

public:
    SourceLocation();
    SourceLocation(CXTranslationUnit cxTranslationUnit,
                   CXSourceLocation cxSourceLocation);

    const Utf8String &filePath() const;
    int line() const;
    int column() const;
    int offset() const;

    SourceLocationContainer toSourceLocationContainer() const;

    CXTranslationUnit tu() const { return m_cxTranslationUnit; }
    CXSourceLocation cx() const { return m_cxSourceLocation; }

private:
    operator CXSourceLocation() const;
    void evaluate() const;

private:
   CXSourceLocation m_cxSourceLocation;
   CXTranslationUnit m_cxTranslationUnit;
   mutable Utf8String m_filePath;
   mutable int m_line = 0;
   mutable int m_column = 0;
   mutable int m_offset = 0;
   mutable bool m_isFilePathNormalized = true;
   mutable bool m_isEvaluated = false;
};

std::ostream &operator<<(std::ostream &os, const SourceLocation &sourceLocation);

} // namespace ClangBackEnd
