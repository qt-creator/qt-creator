/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef CODARUNCONTROL_H
#define CODARUNCONTROL_H

#include "s60runcontrolbase.h"

QT_BEGIN_NAMESPACE
class QMessageBox;
class QWidget;
QT_END_NAMESPACE

namespace Coda {
struct CodaCommandResult;
class CodaDevice;
class CodaEvent;
}

namespace SymbianUtils {
class SymbianDevice;
}

namespace Qt4ProjectManager {

// CodaRunControl configures Coda to run the application
class QT4PROJECTMANAGER_EXPORT CodaRunControl : public S60RunControlBase
{
    Q_OBJECT
public:
    CodaRunControl(ProjectExplorer::RunConfiguration *runConfiguration,
                   ProjectExplorer::RunMode mode);
    virtual ~CodaRunControl();

    virtual bool isRunning() const;
    virtual QIcon icon() const;

    static QMessageBox *createCodaWaitingMessageBox(QWidget *parent = 0);

    using QObject::connect;
    void connect(); // Like start() but doesn't actually launch the program; just hooks up coda.
    void run();

protected:
    virtual bool doStart();
    virtual void doStop();
    virtual bool setupLauncher();

signals:
    void connected();

protected slots:
    void finishRunControl();
    void checkForTimeout();
    void cancelConnection();
    void deviceRemoved(const SymbianUtils::SymbianDevice &device);

private slots:
    void slotError(const QString &error);
    void slotCodaLogMessage(const QString &log);
    void slotCodaEvent(const Coda::CodaEvent &event);

private:
    void initCommunication();

    void handleConnected(const Coda::CodaEvent &event);
    void handleModuleLoadSuspended(const Coda::CodaEvent &event);
    void handleContextSuspended(const Coda::CodaEvent &event);
    void handleContextAdded(const Coda::CodaEvent &event);
    void handleContextRemoved(const Coda::CodaEvent &event);
    void handleLogging(const Coda::CodaEvent &event);
    void handleProcessExited(const Coda::CodaEvent &event);

private:
    void handleCreateProcess(const Coda::CodaCommandResult &result);
    void handleAddListener(const Coda::CodaCommandResult &result);
    void handleDebugSessionStarted(const Coda::CodaCommandResult &result);
    void handleDebugSessionEnded(const Coda::CodaCommandResult &result);
    void handleFindProcesses(const Coda::CodaCommandResult &result);

private:
    enum State {
        StateUninit,
        StateConnecting,
        StateConnected,
        StateDebugSessionStarted,
        StateProcessRunning,
        StateDebugSessionEnded
    };

    QSharedPointer<Coda::CodaDevice> m_codaDevice;

    QString m_address;
    unsigned short m_port;
    QString m_serialPort;
    QString m_runningProcessId;
    QStringList m_codaServices;

    State m_state;
    bool m_stopAfterConnect;
};

} // namespace Qt4ProjectManager

#endif // CODARUNCONTROL_H
