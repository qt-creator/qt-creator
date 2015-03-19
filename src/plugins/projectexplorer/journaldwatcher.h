/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef JOURNALDWATCHER_H
#define JOURNALDWATCHER_H

#include <QByteArray>
#include <QMap>
#include <QObject>

#include <functional>

namespace ProjectExplorer {

class ProjectExplorerPlugin;

class JournaldWatcher : public QObject
{
    Q_OBJECT

public:
    typedef QMap<QByteArray, QByteArray> LogEntry;
    typedef std::function<void(LogEntry)> Subscription;

    ~JournaldWatcher();

    static JournaldWatcher *instance();

    static const QByteArray &machineId();

    static bool subscribe(QObject *subscriber, const Subscription &subscription);
    static void unsubscribe(QObject *subscriber);

signals:
    void journaldOutput(quint64 pid, const QString &message);

private:
    JournaldWatcher();

    void handleEntry();

    static JournaldWatcher *m_instance;

    friend class ProjectExplorerPlugin;
};

} // namespace ProjectExplorer

#endif // JOURNALDWATCHER_H
