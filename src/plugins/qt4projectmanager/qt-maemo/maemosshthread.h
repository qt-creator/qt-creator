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

#ifndef MAEMOSSHTHREAD_H
#define MAEMOSSHTHREAD_H

#include "maemodeviceconfigurations.h"
#include "maemosshconnection.h"

#include <QtCore/QStringList>
#include <QtCore/QThread>

#ifdef USE_SSH_LIB

namespace Qt4ProjectManager {
namespace Internal {

class MaemoSshThread : public QThread
{
    Q_OBJECT
    Q_DISABLE_COPY(MaemoSshThread)
public:
    QString error() const { return m_error; }
    bool hasError() const { return !m_error.isEmpty(); }
    void stop();
    virtual void run();
    ~MaemoSshThread();

signals:
    void connectionEstablished();

protected:
    MaemoSshThread(const MaemoDeviceConfig &devConf);
    virtual void runInternal()=0;
    void setConnection(const MaemoSshConnection::Ptr &connection);
    
    const MaemoDeviceConfig m_devConf;

private:

    QString m_error;
    MaemoSshConnection::Ptr m_connection;
};


class MaemoSshRunner : public MaemoSshThread
{
    Q_OBJECT
    Q_DISABLE_COPY(MaemoSshRunner)
public:
    MaemoSshRunner(const MaemoDeviceConfig &devConf, const QString &command);

signals:
    void remoteOutput(const QString &output);

private:
    virtual void runInternal();

    const QString m_command;
};


class MaemoSshDeployer : public MaemoSshThread
{
    Q_OBJECT
    Q_DISABLE_COPY(MaemoSshDeployer)
public:
    MaemoSshDeployer(const MaemoDeviceConfig &devConf,
        const QStringList &filePaths, const QStringList &targetDirs);

signals:
    void fileCopied(const QString &filePath);

private:
    virtual void runInternal();

    const QStringList m_filePaths;
    const QStringList m_targetDirs;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif

#endif // MAEMOSSHTHREAD_H
