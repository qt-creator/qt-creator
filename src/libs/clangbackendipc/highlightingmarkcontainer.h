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

#ifndef CLANGBACKEND_HIGHLIGHTINGMARKCONTAINER_H
#define CLANGBACKEND_HIGHLIGHTINGMARKCONTAINER_H

#include "clangbackendipc_global.h"
#include <ostream>

namespace ClangBackEnd {

class CMBIPC_EXPORT HighlightingMarkContainer
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const HighlightingMarkContainer &container);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, HighlightingMarkContainer &container);
    friend CMBIPC_EXPORT bool operator==(const HighlightingMarkContainer &first, const HighlightingMarkContainer &second);
public:
    HighlightingMarkContainer() = default;
    HighlightingMarkContainer(uint line, uint column, uint length, HighlightingType type);

    uint line() const;
    uint column() const;
    uint length() const;
    HighlightingType type() const;

private:
    quint32 &typeAsInt();

private:
    uint line_;
    uint column_;
    uint length_;
    HighlightingType type_;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const HighlightingMarkContainer &container);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, HighlightingMarkContainer &container);
CMBIPC_EXPORT bool operator==(const HighlightingMarkContainer &first, const HighlightingMarkContainer &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const HighlightingMarkContainer &container);
CMBIPC_EXPORT void PrintTo(HighlightingType highlightingType, ::std::ostream *os);
void PrintTo(const HighlightingMarkContainer &container, ::std::ostream *os);

} // namespace ClangBackEnd

#endif // CLANGBACKEND_HIGHLIGHTINGMARKCONTAINER_H
