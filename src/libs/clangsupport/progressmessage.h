/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "clangsupport_global.h"

#include <QDataStream>

namespace ClangBackEnd {

class ProgressMessage
{
public:
    ProgressMessage() = default;
    ProgressMessage(ProgressType progressType, int progress, int total)
        : progress(progress)
        , total(total)
        , progressType(progressType)
    {}

    friend QDataStream &operator<<(QDataStream &out, const ProgressMessage &message)
    {
        out << message.progress;
        out << message.total;
        out << static_cast<int>(message.progressType);

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ProgressMessage &message)
    {
        int progressTupe;
        in >> message.progress;
        in >> message.total;
        in >> progressTupe;
        message.progressType = static_cast<ProgressType>(progressTupe);

        return in;
    }

    friend bool operator==(const ProgressMessage &first, const ProgressMessage &second)
    {
        return first.progress == second.progress
            && first.total == second.total;
    }

    ProgressMessage clone() const
    {
        return *this;
    }

public:
    int progress = 0;
    int total = 0;
    ProgressType progressType = ProgressType::Invalid;
};

DECLARE_MESSAGE(ProgressMessage)
} // namespace ClangBackEnd

