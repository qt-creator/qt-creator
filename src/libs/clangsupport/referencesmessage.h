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

class ReferencesMessage
{
public:
    ReferencesMessage() = default;
    ReferencesMessage(const FileContainer &fileContainer,
                      const QVector<SourceRangeContainer> &references,
                      bool isLocalVariable,
                      quint64 ticketNumber)
        : m_fileContainer(fileContainer)
        , m_references(references)
        , m_ticketNumber(ticketNumber)
        , m_isLocalVariable(isLocalVariable)
    {
    }

    const FileContainer &fileContainer() const
    {
        return m_fileContainer;
    }

    bool isLocalVariable() const
    {
        return m_isLocalVariable;
    }

    const QVector<SourceRangeContainer> &references() const
    {
        return m_references;
    }

    quint64 ticketNumber() const
    {
        return m_ticketNumber;
    }

    friend QDataStream &operator<<(QDataStream &out, const ReferencesMessage &message)
    {
        out << message.m_fileContainer;
        out << message.m_isLocalVariable;
        out << message.m_references;
        out << message.m_ticketNumber;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ReferencesMessage &message)
    {
        in >> message.m_fileContainer;
        in >> message.m_isLocalVariable;
        in >> message.m_references;
        in >> message.m_ticketNumber;

        return in;
    }

    friend bool operator==(const ReferencesMessage &first, const ReferencesMessage &second)
    {
        return first.m_ticketNumber == second.m_ticketNumber
            && first.m_isLocalVariable == second.m_isLocalVariable
            && first.m_fileContainer == second.m_fileContainer
            && first.m_references == second.m_references;
    }

    friend CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const ReferencesMessage &message);
    friend std::ostream &operator<<(std::ostream &os, const ReferencesMessage &message);

private:
    FileContainer m_fileContainer;
    QVector<SourceRangeContainer> m_references;
    quint64 m_ticketNumber = 0;
    bool m_isLocalVariable = false;
};

DECLARE_MESSAGE(ReferencesMessage)
} // namespace ClangBackEnd
