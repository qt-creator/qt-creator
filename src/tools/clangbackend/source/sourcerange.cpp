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

#include "sourcerange.h"

#include <sourcerangecontainer.h>

#include <ostream>

#include <utils/qtcassert.h>

namespace ClangBackEnd {

SourceRange::SourceRange()
    : cxSourceRange(clang_getNullRange())
{
}

SourceRange::SourceRange(const SourceLocation &start, const SourceLocation &end)
    : cxSourceRange(clang_getRange(start, end)),
      cxTranslationUnit(start.m_cxTranslationUnit)
{
}

bool SourceRange::isNull() const
{
    return clang_Range_isNull(cxSourceRange);
}

bool SourceRange::isValid() const
{
    return !isNull() && start().offset() < end().offset();
}

SourceLocation SourceRange::start() const
{
    return {cxTranslationUnit, clang_getRangeStart(cxSourceRange)};
}

SourceLocation SourceRange::end() const
{
    return {cxTranslationUnit, clang_getRangeEnd(cxSourceRange)};
}

bool SourceRange::contains(int line, int column) const
{
    const SourceLocation start_ = start();
    const SourceLocation end_ = end();

    if (line < start_.line() || line > end_.line())
        return false;
    if (line == start_.line() && column < start_.column())
        return false;
    if (line == end_.line() && column > end_.column())
        return false;
    return true;
}

SourceRangeContainer SourceRange::toSourceRangeContainer() const
{
    return SourceRangeContainer(start().toSourceLocationContainer(),
                                end().toSourceLocationContainer());
}

ClangBackEnd::SourceRange::operator SourceRangeContainer() const
{
    return toSourceRangeContainer();
}

ClangBackEnd::SourceRange::operator CXSourceRange() const
{
    return cxSourceRange;
}

SourceRange::SourceRange(CXTranslationUnit translationUnit, CXSourceRange cxSourceRange)
    : cxSourceRange(cxSourceRange),
      cxTranslationUnit(translationUnit)
{
}

bool operator==(const SourceRange &first, const SourceRange &second)
{
    return clang_equalRanges(first.cxSourceRange, second.cxSourceRange);
}

std::ostream &operator<<(std::ostream &os, const SourceRange &sourceRange)
{
    os << "["
       << sourceRange.start() << ", "
       << sourceRange.end()
       << "]";

    return os;
}
} // namespace ClangBackEnd

