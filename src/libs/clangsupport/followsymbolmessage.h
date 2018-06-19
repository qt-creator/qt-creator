/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include "sourcerangecontainer.h"

#include <QDataStream>
#include <QVector>

namespace ClangBackEnd {

class FollowSymbolResult
{
public:
    FollowSymbolResult() = default;
    FollowSymbolResult(SourceRangeContainer range)
        : range(std::move(range))
    {}
    FollowSymbolResult(SourceRangeContainer range, bool isResultOnlyForFallBack)
        : range(std::move(range))
        , isResultOnlyForFallBack(isResultOnlyForFallBack)
    {}

    friend QDataStream &operator<<(QDataStream &out, const FollowSymbolResult &container)
    {
        out << container.range;
        out << container.isResultOnlyForFallBack;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FollowSymbolResult &container)
    {
        in >> container.range;
        in >> container.isResultOnlyForFallBack;

        return in;
    }

    friend bool operator==(const FollowSymbolResult &first, const FollowSymbolResult &second)
    {
        return first.range == second.range
                && first.isResultOnlyForFallBack == second.isResultOnlyForFallBack;
    }

    SourceRangeContainer range;
    bool isResultOnlyForFallBack = false;
};

class FollowSymbolMessage
{
public:
    FollowSymbolMessage() = default;
    FollowSymbolMessage(const FileContainer &fileContainer,
                        const FollowSymbolResult &result,
                        quint64 ticketNumber)
        : fileContainer(fileContainer)
        , result(result)
        , ticketNumber(ticketNumber)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const FollowSymbolMessage &message)
    {
        out << message.fileContainer;
        out << message.result;
        out << message.ticketNumber;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, FollowSymbolMessage &message)
    {
        in >> message.fileContainer;
        in >> message.result;
        in >> message.ticketNumber;
        return in;
    }

    friend bool operator==(const FollowSymbolMessage &first, const FollowSymbolMessage &second)
    {
        return first.ticketNumber == second.ticketNumber
                && first.fileContainer == second.fileContainer
                && first.result == second.result;
    }

public:
    FileContainer fileContainer;
    FollowSymbolResult result;
    quint64 ticketNumber = 0;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const FollowSymbolResult &result);
CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const FollowSymbolMessage &message);

DECLARE_MESSAGE(FollowSymbolMessage);

} // namespace ClangBackEnd
