/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
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
**************************************************************************/

#ifndef CODARUNCONTROL_H
#define CODARUNCONTROL_H

#include "s60runcontrolbase.h"

QT_BEGIN_NAMESPACE
class QMessageBox;
class QWidget;
QT_END_NAMESPACE

namespace tcftrk {
struct TcfTrkCommandResult;
class TcfTrkDevice;
class TcfTrkEvent;
}

namespace Qt4ProjectManager {
namespace Internal {

// CodaRunControl configures tcftrk to run the application
class CodaRunControl : public S60RunControlBase
{
    Q_OBJECT
public:
    CodaRunControl(ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode);
    ~CodaRunControl();
    virtual bool isRunning() const;

    static QMessageBox *createCodaWaitingMessageBox(QWidget *parent = 0);

protected:
    virtual bool doStart();
    virtual void doStop();
    virtual bool setupLauncher();

protected slots:
    void finishRunControl();
    void checkForTimeout();
    void cancelConnection();

private slots:
    void slotError(const QString &error);
    void slotTrkLogMessage(const QString &log);
    void slotTcftrkEvent(const tcftrk::TcfTrkEvent &event);
    void slotSerialPong(const QString &message);

private:
    void initCommunication();

    void handleModuleLoadSuspended(const tcftrk::TcfTrkEvent &event);
    void handleContextSuspended(const tcftrk::TcfTrkEvent &event);
    void handleContextAdded(const tcftrk::TcfTrkEvent &event);
    void handleContextRemoved(const tcftrk::TcfTrkEvent &event);
    void handleLogging(const tcftrk::TcfTrkEvent &event);

private:
    void handleCreateProcess(const tcftrk::TcfTrkCommandResult &result);
    void handleAddListener(const tcftrk::TcfTrkCommandResult &result);
    void handleFindProcesses(const tcftrk::TcfTrkCommandResult &result);

private:
    enum State {
        StateUninit,
        StateConnecting,
        StateConnected,
        StateProcessRunning
    };

    tcftrk::TcfTrkDevice *m_tcfTrkDevice;

    QString m_address;
    unsigned short m_port;
    QString m_runningProcessId;

    State m_state;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // CODARUNCONTROL_H
