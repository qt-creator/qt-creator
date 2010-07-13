#ifndef MAEMODEPLOYSTEP_H
#define MAEMODEPLOYSTEP_H

#include "maemodeployable.h"
#include "maemodeviceconfigurations.h"

#include <coreplugin/ssh/sftpdefs.h>
#include <projectexplorer/buildstep.h>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QEventLoop;
QT_END_NAMESPACE

namespace Core {
class SftpChannel;
class SshConnection;
class SshRemoteProcess;
}

namespace Qt4ProjectManager {
namespace Internal {
class MaemoPackageCreationStep;

class MaemoDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class MaemoDeployStepFactory;
public:
    MaemoDeployStep(ProjectExplorer::BuildConfiguration *bc);
    virtual ~MaemoDeployStep();
    MaemoDeviceConfig deviceConfig() const;
    bool currentlyNeedsDeployment(const QString &host,
        const MaemoDeployable &deployable) const;
    void setDeployed(const QString &host, const MaemoDeployable &deployable);

signals:
    void done();
    void error();

private slots:
    void start();
    void handleConnected();
    void handleConnectionFailure();
    void handleSftpChannelInitialized();
    void handleSftpChannelInitializationFailed(const QString &error);
    void handleSftpJobFinished(Core::SftpJobId job, const QString &error);
    void handleLinkProcessFinished(int exitStatus);
    void handleInstallationFinished(int exitStatus);
    void handleInstallerOutput(const QByteArray &output);
    void handleInstallerErrorOutput(const QByteArray &output);

private:
    MaemoDeployStep(ProjectExplorer::BuildConfiguration *bc,
        MaemoDeployStep *other);
    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const { return true; }
    virtual QVariantMap toMap() const;
    virtual bool fromMap(const QVariantMap &map);

    void ctor();
    void stop();
    void raiseError(const QString &error);
    void writeOutput(const QString &text,
        const QTextCharFormat &format = QTextCharFormat());
    void addDeployTimesToMap(QVariantMap &map) const;
    void getDeployTimesFromMap(const QVariantMap &map);
    const MaemoPackageCreationStep *packagingStep() const;
    bool deploy(const MaemoDeployable &deployable);
    QString uploadDir() const;
    QString packageFileName() const;
    QString packageFilePath() const;

    static const QLatin1String Id;

    QSharedPointer<Core::SshConnection> m_connection;
    QSharedPointer<Core::SftpChannel> m_uploader;
    QSharedPointer<Core::SshRemoteProcess> m_installer;
    typedef QPair<MaemoDeployable, QString> DeployInfo;
    QMap<Core::SftpJobId, DeployInfo> m_uploadsInProgress;
    QMap<QSharedPointer<Core::SshRemoteProcess>, MaemoDeployable> m_linksInProgress;
    bool m_needsInstall;
    bool m_stopped;
    typedef QPair<MaemoDeployable, QString> DeployablePerHost;
    QHash<DeployablePerHost, QDateTime> m_lastDeployed;
};

class MaemoDeployEventHandler : public QObject
{
    Q_OBJECT
public:
    MaemoDeployEventHandler(MaemoDeployStep *deployStep,
        QFutureInterface<bool> &future);

private slots:
    void handleDeployingDone();
    void handleDeployingFailed();
    void checkForCanceled();

private:
    const MaemoDeployStep * const m_deployStep;
    const QFutureInterface<bool> m_future;
    QEventLoop * const m_eventLoop;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEPLOYSTEP_H
