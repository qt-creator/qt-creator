// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QString>
#include <QDataStream>

namespace QmlDesigner {

class ChangeLanguageCommand
{
public:
    ChangeLanguageCommand() = default;
    ChangeLanguageCommand(QString language) : language(std::move(language)) {
    }

    friend QDataStream &operator<<(QDataStream &out, const ChangeLanguageCommand &command)
    {
        return out << command.language;
    }
    friend QDataStream &operator>>(QDataStream &in, ChangeLanguageCommand &command)
    {
        return in >> command.language;
    }

    friend QDebug operator<<(QDebug debug, const ChangeLanguageCommand &command);

    QString language;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangeLanguageCommand)
