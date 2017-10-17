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

#include "filecontainer.h"

#include <QVector>

namespace ClangBackEnd {

class UnregisterTranslationUnitsForEditorMessage
{
public:
    UnregisterTranslationUnitsForEditorMessage() = default;
    UnregisterTranslationUnitsForEditorMessage(const QVector<FileContainer> &fileContainers)
        : m_fileContainers(fileContainers)
    {
    }

    const QVector<FileContainer> &fileContainers() const
    {
        return m_fileContainers;
    }

    friend QDataStream &operator<<(QDataStream &out, const UnregisterTranslationUnitsForEditorMessage &message)
    {
        out << message.m_fileContainers;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, UnregisterTranslationUnitsForEditorMessage &message)
    {
        in >> message.m_fileContainers;

        return in;
    }

    friend bool operator==(const UnregisterTranslationUnitsForEditorMessage &first, const UnregisterTranslationUnitsForEditorMessage &second)
    {
        return first.m_fileContainers == second.m_fileContainers;
    }

    friend std::ostream &operator<<(std::ostream &os, const UnregisterTranslationUnitsForEditorMessage &message);

private:
    QVector<FileContainer> m_fileContainers;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const UnregisterTranslationUnitsForEditorMessage &message);

DECLARE_MESSAGE(UnregisterTranslationUnitsForEditorMessage);
} // namespace ClangBackEnd
