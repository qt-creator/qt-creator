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

#ifndef  CLANGBACKEND_REQUESTHIGHLIGHTING_H
#define  CLANGBACKEND_REQUESTHIGHLIGHTING_H

#include "filecontainer.h"

namespace ClangBackEnd {

class CMBIPC_EXPORT RequestHighlightingMessage
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const RequestHighlightingMessage &message);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, RequestHighlightingMessage &message);
    friend CMBIPC_EXPORT bool operator==(const RequestHighlightingMessage &first, const RequestHighlightingMessage &second);
    friend CMBIPC_EXPORT QDebug operator<<(QDebug debug, const RequestHighlightingMessage &message);
    friend void PrintTo(const RequestHighlightingMessage &message, ::std::ostream* os);

public:
    RequestHighlightingMessage() = default;
    RequestHighlightingMessage(const FileContainer &fileContainer);

    const FileContainer fileContainer() const;

private:
    FileContainer fileContainer_;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const RequestHighlightingMessage &message);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, RequestHighlightingMessage &message);
CMBIPC_EXPORT bool operator==(const RequestHighlightingMessage &first, const RequestHighlightingMessage &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const RequestHighlightingMessage &message);
void PrintTo(const RequestHighlightingMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(RequestHighlightingMessage)
} // namespace ClangBackEnd

#endif //  CLANGBACKEND_REQUESTHIGHLIGHTING_H
