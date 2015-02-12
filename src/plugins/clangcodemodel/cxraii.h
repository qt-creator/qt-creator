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

#ifndef CXRAII_H
#define CXRAII_H

#include <clang-c/Index.h>

// Simple RAII types for their CX correspondings

namespace ClangCodeModel {
namespace Internal {

template <class CXType_T>
struct ScopedCXType
{
protected:
    typedef void (*DisposeFunction)(CXType_T);

    ScopedCXType(DisposeFunction f)
        : m_cx(0), m_disposeFunction(f)
    {}
    ScopedCXType(const CXType_T &cx, DisposeFunction f)
        : m_cx(cx) , m_disposeFunction(f)
    {}

public:
    ~ScopedCXType()
    {
        dispose();
    }

    operator CXType_T() const { return m_cx; }
    bool operator!() const { return !m_cx; }
    bool isNull() const { return !m_cx; }
    void reset(const CXType_T &cx)
    {
        dispose();
        m_cx = cx;
    }

private:
    ScopedCXType(const ScopedCXType &);
    const ScopedCXType &operator=(const ScopedCXType &);

    void dispose()
    {
        if (m_cx)
            m_disposeFunction(m_cx);
    }

    CXType_T m_cx;
    DisposeFunction m_disposeFunction;
};

struct ScopedCXIndex : ScopedCXType<CXIndex>
{
    ScopedCXIndex()
        : ScopedCXType<CXIndex>(&clang_disposeIndex)
    {}
    ScopedCXIndex(const CXIndex &index)
        : ScopedCXType<CXIndex>(index, &clang_disposeIndex)
    {}
};

struct ScopedCXTranslationUnit : ScopedCXType<CXTranslationUnit>
{
    ScopedCXTranslationUnit()
        : ScopedCXType<CXTranslationUnit>(&clang_disposeTranslationUnit)
    {}
    ScopedCXTranslationUnit(const CXTranslationUnit &unit)
        : ScopedCXType<CXTranslationUnit>(unit, &clang_disposeTranslationUnit)
    {}
};

struct ScopedCXDiagnostic : ScopedCXType<CXDiagnostic>
{
    ScopedCXDiagnostic()
        : ScopedCXType<CXDiagnostic>(&clang_disposeDiagnostic)
    {}
    ScopedCXDiagnostic(const CXDiagnostic &diagnostic)
        : ScopedCXType<CXDiagnostic>(diagnostic, &clang_disposeDiagnostic)
    {}
};

struct ScopedCXDiagnosticSet : ScopedCXType<CXDiagnostic>
{
    ScopedCXDiagnosticSet()
        : ScopedCXType<CXDiagnosticSet>(&clang_disposeDiagnosticSet)
    {}
    ScopedCXDiagnosticSet(const CXDiagnostic &diagnostic)
        : ScopedCXType<CXDiagnosticSet>(diagnostic, &clang_disposeDiagnosticSet)
    {}
};

struct ScopedCXCodeCompleteResults : ScopedCXType<CXCodeCompleteResults*>
{
    ScopedCXCodeCompleteResults()
        : ScopedCXType<CXCodeCompleteResults*>(&clang_disposeCodeCompleteResults)
    {}
    ScopedCXCodeCompleteResults(CXCodeCompleteResults *results)
        : ScopedCXType<CXCodeCompleteResults*>(results, &clang_disposeCodeCompleteResults)
    {}

    unsigned size() const
    {
        return static_cast<CXCodeCompleteResults *>(*this)->NumResults;
    }

    const CXCompletionResult &completionAt(unsigned i)
    {
        return static_cast<CXCodeCompleteResults *>(*this)->Results[i];
    }
};

} // Internal
} // ClangCodeModel

#endif // CXRAII_H
