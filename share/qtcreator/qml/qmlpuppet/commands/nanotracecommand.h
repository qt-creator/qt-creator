/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

