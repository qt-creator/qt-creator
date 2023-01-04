// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDataStream>
#include <QDebug>
#include <QMetaType>
#include <QString>

namespace QmlDesigner {

class StartNanotraceCommand
{
public:
    StartNanotraceCommand() = default;
    StartNanotraceCommand(const QString &path);
    QString path() const;
    friend QDataStream &operator<<(QDataStream &out, const StartNanotraceCommand &command);
    friend QDataStream &operator>>(QDataStream &in, StartNanotraceCommand &command);
    friend QDebug operator <<(QDebug debug, const StartNanotraceCommand &command);
private:
    QString m_filePath;
};


class EndNanotraceCommand
{
public:
    EndNanotraceCommand() = default;
    friend QDataStream &operator<<(QDataStream &out, const EndNanotraceCommand &command);
    friend QDataStream &operator>>(QDataStream &in, EndNanotraceCommand &command);
    friend QDebug operator <<(QDebug debug, const EndNanotraceCommand &command);
};


class SyncNanotraceCommand
{
public:
    SyncNanotraceCommand() = default;
    SyncNanotraceCommand(const QString& name);
    QString name() const;
    friend QDataStream &operator>>(QDataStream &in, SyncNanotraceCommand &command);
    friend QDataStream &operator<<(QDataStream &out, const SyncNanotraceCommand &command);
    friend QDebug operator <<(QDebug debug, const SyncNanotraceCommand &command);
private:
    QString m_name;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::StartNanotraceCommand)
Q_DECLARE_METATYPE(QmlDesigner::EndNanotraceCommand)
Q_DECLARE_METATYPE(QmlDesigner::SyncNanotraceCommand)

