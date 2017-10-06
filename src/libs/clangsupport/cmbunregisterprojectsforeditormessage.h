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

#include "clangsupport_global.h"

#include <utf8stringvector.h>

#include <QDataStream>

namespace ClangBackEnd {

class CLANGSUPPORT_EXPORT UnregisterProjectPartsForEditorMessage
{
public:
    UnregisterProjectPartsForEditorMessage() = default;
    UnregisterProjectPartsForEditorMessage(const Utf8StringVector &projectPartIds)
        : m_projectPartIds(projectPartIds)
    {
    }

    const Utf8StringVector &projectPartIds() const
    {
        return m_projectPartIds;
    }

    friend QDataStream &operator<<(QDataStream &out, const UnregisterProjectPartsForEditorMessage &message)
    {
        out << message.m_projectPartIds;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, UnregisterProjectPartsForEditorMessage &message)
    {
        in >> message.m_projectPartIds;

        return in;
    }

    friend bool operator==(const UnregisterProjectPartsForEditorMessage &first, const UnregisterProjectPartsForEditorMessage &second)
    {
        return first.m_projectPartIds == second.m_projectPartIds;
    }

private:
    Utf8StringVector m_projectPartIds;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const UnregisterProjectPartsForEditorMessage &message);
std::ostream &operator<<(std::ostream &os, const UnregisterProjectPartsForEditorMessage &message);

DECLARE_MESSAGE(UnregisterProjectPartsForEditorMessage);
} // namespace ClangBackEnd
