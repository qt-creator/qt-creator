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

#ifndef MAEMORUNCONTROL_H
#define MAEMORUNCONTROL_H

#include <projectexplorer/runconfiguration.h>

#include <QtCore/QString>

namespace Qt4ProjectManager {
namespace Internal {

class MaemoRunConfiguration;
class MaemoSshRunner;

class MaemoRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT
public:
    explicit MaemoRunControl(ProjectExplorer::RunConfiguration *runConfig);
    virtual ~MaemoRunControl();

    virtual void start();
    virtual StopResult stop();
    virtual bool isRunning() const;

private slots:
    void startExecution();
    void handleSshError(const QString &error);
    void handleRemoteProcessStarted() {}
    void handleRemoteProcessFinished(qint64 exitCode);
    void handleRemoteOutput(const QByteArray &output);
    void handleRemoteErrorOutput(const QByteArray &output);
    void handleProgressReport(const QString &progressString);
    void handleMountDebugOutput(const QString &output);

private:
    void setFinished();
    void handleError(const QString &errString);

    MaemoSshRunner * const m_runner;
    bool m_running;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMORUNCONTROL_H
