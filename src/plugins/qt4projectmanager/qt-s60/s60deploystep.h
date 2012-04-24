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

#ifndef S60DeployStep_H
#define S60DeployStep_H

#include <projectexplorer/buildstep.h>

#include <QString>

QT_FORWARD_DECLARE_CLASS(QEventLoop)
QT_FORWARD_DECLARE_CLASS(QFile)

namespace SymbianUtils {
class SymbianDevice;
}

namespace Coda {
    struct CodaCommandResult;
    class CodaDevice;
    class CodaEvent;
}

namespace ProjectExplorer {
class IOutputParser;
}

namespace Qt4ProjectManager {

class S60DeviceRunConfiguration;

namespace Internal {

class BuildConfiguration;
struct CommunicationChannel;

class S60DeployStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT
public:
    explicit S60DeployStepFactory(QObject *parent = 0);
    virtual ~S60DeployStepFactory();

    // used to show the list of possible additons to a target, returns a list of types
    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    // used to translate the types to names to display to the user
    QString displayNameForId(const Core::Id id) const;

    bool canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id);
    // used to recreate the runConfigurations when restoring settings
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product);

    bool canHandle(ProjectExplorer::BuildStepList *parent) const;
};

class S60DeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
public:
    friend class S60DeployStepFactory;

    explicit S60DeployStep(ProjectExplorer::BuildStepList *parent);

    virtual ~S60DeployStep();

    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    virtual QVariantMap toMap() const;

protected:
    virtual bool fromMap(const QVariantMap &map);

protected slots:
    void deviceRemoved(const SymbianUtils::SymbianDevice &);

private slots:
    void checkForCancel();
    void checkForTimeout();
    void timeout();

    void slotError(const QString &error);
    void slotCodaLogMessage(const QString &log);
    void slotSerialPong(const QString &message);
    void slotCodaEvent(const Coda::CodaEvent &event);

    void startInstalling();
    void startTransferring();

    void deploymentFinished(bool success);
    void slotWaitingForCodaClosed(int result);
    void showManualInstallationInfo();

    void setCopyProgress(int progress);

    void updateProgress(int progress);

signals:
    void s60DeploymentFinished(bool success = true);
    void finishNow(bool success = true);

    void allFilesSent();
    void allFilesInstalled();

    void codaConnected();

    void manualInstallation();
    void copyProgressChanged(int progress);

private:
    S60DeployStep(ProjectExplorer::BuildStepList *parent,
                  S60DeployStep *bs);
    void ctor();

    void start();
    void stop();
    void startDeployment();
    bool processPackageName(QString &errorMessage);
    void setupConnections();
    void appendMessage(const QString &error, bool isError);
    void reportError(const QString &error);

    void handleConnected();
    void handleSymbianInstall(const Coda::CodaCommandResult &result);
    void handleFileSystemOpen(const Coda::CodaCommandResult &result);
    void handleFileSystemWrite(const Coda::CodaCommandResult &result);
    void closeFiles();
    void putSendNextChunk();
    void handleFileSystemClose(const Coda::CodaCommandResult &result);

    void initFileSending();
    void initFileInstallation();
    int copyProgress() const;

    enum State {
        StateUninit,
        StateConnecting,
        StateConnected,
        StateSendingData,
        StateInstalling,
        StateFinished
    };

    inline void setState(State state) { m_state = state; }
    inline State state() { return m_state; }

    QString m_serialPortName;
    QString m_serialPortFriendlyName;
    QStringList m_packageFileNamesWithTarget; // Support for 4.6.1
    QStringList m_signedPackages;
    QString m_address;
    unsigned short m_port;

    QTimer *m_timer;
    QTimer* m_timeoutTimer;

    QFutureInterface<bool> *m_futureInterface; //not owned

    QSharedPointer<Coda::CodaDevice> m_codaDevice;

    QEventLoop *m_eventLoop;
    bool m_deployResult;
    char m_installationDrive;
    bool m_silentInstall;

    State m_state;
    bool m_putWriteOk;
    QScopedPointer<QFile> m_putFile;
    quint64 m_putLastChunkSize;
    QByteArray m_remoteFileHandle;
    quint64 m_putChunkSize;
    int m_currentFileIndex;
    int m_channel;
    volatile bool m_deployCanceled;
    int m_copyProgress;
};

} // Internal
} // Qt4ProjectManager

#endif // S60DeployStep_H
