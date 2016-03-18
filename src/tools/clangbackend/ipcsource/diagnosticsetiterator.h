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

#include <iterator>

#include <clang-c/Index.h>

namespace ClangBackEnd {

using uint = unsigned int;

class DiagnosticSet;
class Diagnostic;

class DiagnosticSetIterator : public std::iterator<std::random_access_iterator_tag, Diagnostic, uint>
{
public:
    DiagnosticSetIterator(CXDiagnosticSet cxDiagnosticSet, uint index)
        : cxDiagnosticSet(cxDiagnosticSet),
          index(index)
    {}

    DiagnosticSetIterator(const DiagnosticSetIterator &other)
        : cxDiagnosticSet(other.cxDiagnosticSet),
          index(other.index)
    {}

    DiagnosticSetIterator& operator++()
    {
        ++index;
        return *this;
    }

    DiagnosticSetIterator operator++(int)
    {
        uint oldIndex = index++;
        return DiagnosticSetIterator(cxDiagnosticSet, oldIndex);
    }

    bool operator==(const DiagnosticSetIterator &other)
    {
        return index == other.index && cxDiagnosticSet == other.cxDiagnosticSet;
    }

    bool operator!=(const DiagnosticSetIterator &other)
    {
        return index != other.index || cxDiagnosticSet != other.cxDiagnosticSet;
    }

    Diagnostic operator*()
    {
        return Diagnostic(clang_getDiagnosticInSet(cxDiagnosticSet, index));
    }

private:
    CXDiagnosticSet cxDiagnosticSet;
    uint index;
};

}
