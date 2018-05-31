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

#include "filecontainer.h"

namespace ClangBackEnd {

class RequestAnnotationsMessage
{
public:
    RequestAnnotationsMessage() = default;
    RequestAnnotationsMessage(const FileContainer &fileContainer)
        : fileContainer(fileContainer)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const RequestAnnotationsMessage &message)
    {
        out << message.fileContainer;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RequestAnnotationsMessage &message)
    {
        in >> message.fileContainer;

        return in;
    }

    friend bool operator==(const RequestAnnotationsMessage &first,
                           const RequestAnnotationsMessage &second)
    {
        return first.fileContainer == second.fileContainer;
    }

public:
    FileContainer fileContainer;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const RequestAnnotationsMessage &message);

DECLARE_MESSAGE(RequestAnnotationsMessage);
} // namespace ClangBackEnd
