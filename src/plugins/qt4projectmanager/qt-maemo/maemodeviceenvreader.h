/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef DEVICEENVREADER_H
#define DEVICEENVREADER_H

#include "maemodeviceconfigurations.h"

#include <utils/environment.h>

#include <QtCore/QObject>

namespace Core {
    class SshRemoteProcessRunner;
}

namespace Qt4ProjectManager {
    namespace Internal {

class MaemoRunConfiguration;

class MaemoDeviceEnvReader : public QObject
{
    Q_OBJECT
public:
    MaemoDeviceEnvReader(QObject *parent, MaemoRunConfiguration *config);
    ~MaemoDeviceEnvReader();

    void start();
    void stop();

    Utils::Environment deviceEnvironment() const { return m_env; }

signals:
    void finished();
    void error(const QString &error);

private slots:
    void handleConnectionFailure();
    void handleCurrentDeviceConfigChanged();

    void remoteProcessFinished(int exitCode);
    void remoteOutput(const QByteArray &data);
    void remoteErrorOutput(const QByteArray &data);

private:
    void setFinished();

    bool m_stop;
    QString m_remoteOutput;
    QByteArray m_remoteErrorOutput;
    Utils::Environment m_env;
    MaemoDeviceConfig m_devConfig;
    MaemoRunConfiguration *m_runConfig;
    QSharedPointer<Core::SshRemoteProcessRunner> m_remoteProcessRunner;
};

    }   // Internal
}   // Qt4ProjectManager

#endif  // DEVICEENVREADER_H
