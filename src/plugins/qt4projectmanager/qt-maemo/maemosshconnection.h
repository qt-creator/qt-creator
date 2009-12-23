/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAEMOSSHCONNECTION_H
#define MAEMOSSHCONNECTION_H

#ifdef USE_SSH_LIB

#include <QtCore/QCoreApplication>
#include <QtCore/QScopedPointer>
#include <QtCore/QSharedPointer>

class ne7ssh;

namespace Qt4ProjectManager {
namespace Internal {

class MaemoDeviceConfig;

class MaemoSshException
{
public:
    MaemoSshException(const QString &error) : m_error(error) {}
    const QString &error() const { return m_error; }
private:
    const QString m_error;
};

class MaemoSshConnection : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MaemoSshConnection)
    friend class MaemoSshFacility;
public:
    typedef QSharedPointer<MaemoSshConnection> Ptr;

    static Ptr connect(const MaemoDeviceConfig &devConf, bool shell);
    void runCommand(const QString &command);
    void stopCommand();
    ~MaemoSshConnection();

signals:
    void remoteOutput(const QString &output);

private:
    MaemoSshConnection(const MaemoDeviceConfig &devConf, bool shell);

    int m_channel;
    const char *m_prompt;
    volatile bool m_stopRequested;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif

#endif // MAEMOSSHCONNECTION_H
