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

#ifndef CLANGBACKEND_FIXITCONTAINER_H
#define CLANGBACKEND_FIXITCONTAINER_H

#include "sourcerangecontainer.h"

namespace ClangBackEnd {

class CMBIPC_EXPORT FixItContainer
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const FixItContainer &container);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, FixItContainer &container);
    friend CMBIPC_EXPORT bool operator==(const FixItContainer &first, const FixItContainer &second);
public:
    FixItContainer() = default;
    FixItContainer(const Utf8String &text,
                   const SourceRangeContainer &range);

    const Utf8String &text() const;
    const SourceRangeContainer &range() const;

private:
    SourceRangeContainer range_;
    Utf8String text_;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const FixItContainer &container);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, FixItContainer &container);
CMBIPC_EXPORT bool operator==(const FixItContainer &first, const FixItContainer &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const FixItContainer &container);
void PrintTo(const FixItContainer &container, ::std::ostream* os);

} // namespace ClangBackEnd

Q_DECLARE_METATYPE(ClangBackEnd::FixItContainer)

#endif // CLANGBACKEND_FIXITCONTAINER_H
