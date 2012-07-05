/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef S60RUNCONTROLBASE_H
#define S60RUNCONTROLBASE_H

#include <qt4projectmanager/qt4projectmanager_global.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/toolchain.h>

#include <QFutureInterface>

namespace Qt4ProjectManager {

class QT4PROJECTMANAGER_EXPORT S60RunControlBase : public ProjectExplorer::RunControl
{
    Q_OBJECT
public:
    S60RunControlBase(ProjectExplorer::RunConfiguration *runConfiguration,
                      ProjectExplorer::RunMode mode);
    ~S60RunControlBase();

    virtual void start();
    virtual StopResult stop();
    virtual bool promptToStop(bool *optionalPrompt = 0) const;

    static QString msgListFile(const QString &file);

protected:
    virtual bool doStart() = 0;
    virtual void doStop() = 0;
    virtual bool setupLauncher() = 0;

protected:
    quint32 executableUid() const;
    QString executableName() const;
    const QString &targetName() const;
    const QString &commandLineArguments() const;
    const QString &executableFileName() const;
    bool runSmartInstaller() const;
    char installationDrive() const;

    void setProgress(int value);
    void cancelProgress();
    int maxProgress() const;

private:
    void startLaunching();


protected slots:
    void reportLaunchFinished();

private slots:
    void handleFinished();

private:
    QFutureInterface<void> *m_launchProgress;
    quint32 m_executableUid;
    QString m_targetName;
    QString m_commandLineArguments;
    QString m_executableFileName;
    bool m_runSmartInstaller;
    char m_installationDrive;
};

} // namespace Qt4ProjectManager

#endif // S60RUNCONTROLBASE_H
