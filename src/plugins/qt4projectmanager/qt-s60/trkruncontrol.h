/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef TRKRUNCONTROL_H
#define TRKRUNCONTROL_H

#include "s60runcontrolbase.h"

QT_BEGIN_NAMESPACE
class QMessageBox;
class QWidget;
QT_END_NAMESPACE

namespace SymbianUtils {
class SymbianDevice;
}

namespace trk {
class Launcher;
}

namespace Qt4ProjectManager {
namespace Internal {

// TrkRunControl configures launcher to run the application
class TrkRunControl : public S60RunControlBase
{
    Q_OBJECT
public:
    TrkRunControl(ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode);
    ~TrkRunControl();
    virtual bool isRunning() const;

    static QMessageBox *createTrkWaitingMessageBox(const QString &port, QWidget *parent = 0);

protected:
    virtual bool doStart();
    virtual void doStop();
    virtual bool setupLauncher();
    virtual void initLauncher(const QString &executable, trk::Launcher *);

protected slots:
    void printApplicationOutput(const QString &output, bool onStdErr);
    void printApplicationOutput(const QString &output);
    void printStartingNotice();
    void applicationRunNotice(uint pid);
    void applicationRunFailedNotice(const QString &errorMessage);
    void deviceRemoved(const SymbianUtils::SymbianDevice &);

private slots:
    void processStopped(uint pc, uint pid, uint tid, const QString &reason);
    void printConnectFailed(const QString &errorMessage);
    void launcherFinished();
    void slotLauncherStateChanged(int);
    void slotWaitingForTrkClosed();

private:
    void initCommunication();

private:
    trk::Launcher *m_launcher;
    QString m_serialPortName;
    QString m_serialPortFriendlyName;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // TRKRUNCONTROL_H
