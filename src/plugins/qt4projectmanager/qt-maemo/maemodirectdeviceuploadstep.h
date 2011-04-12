#ifndef MAEMODIRECTDEVICEUPLOADSTEP_H
#define MAEMODIRECTDEVICEUPLOADSTEP_H

#include "abstractmaemodeploystep.h"

#include <utils/ssh/sftpdefs.h>

#include <QtCore/QList>
#include <QtCore/QSharedPointer>

namespace Utils {
class SshRemoteProcess;
class SftpChannel;
}

namespace Qt4ProjectManager {
namespace Internal {
class MaemoDeployable;

class MaemoDirectDeviceUploadStep : public AbstractMaemoDeployStep
{
    Q_OBJECT
public:
    MaemoDirectDeviceUploadStep(ProjectExplorer::BuildStepList *bc);
    MaemoDirectDeviceUploadStep(ProjectExplorer::BuildStepList *bc,
        MaemoDirectDeviceUploadStep *other);
    ~MaemoDirectDeviceUploadStep();

    static const QString Id;
    static const QString DisplayName;

private slots:
    void handleSftpInitialized();
    void handleSftpInitializationFailed(const QString &errorMessage);
    void handleUploadFinished(Utils::SftpJobId jobId, const QString &errorMsg);
    void handleMkdirFinished(int exitStatus);

private:
    enum ExtendedState { Inactive, InitializingSftp, Uploading };

    virtual bool isDeploymentPossibleInternal(QString &whynot) const;
    virtual bool isDeploymentNeeded(const QString &hostName) const;
    virtual void startInternal();
    virtual void stopInternal();
    virtual const AbstractMaemoPackageCreationStep *packagingStep() const { return 0; }

    void ctor();
    void setFinished();
    void checkDeploymentNeeded(const QString &hostName,
        const MaemoDeployable &deployable) const;
    void uploadNextFile();

    QSharedPointer<Utils::SftpChannel> m_uploader;
    QSharedPointer<Utils::SshRemoteProcess> m_mkdirProc;
    mutable QList<MaemoDeployable> m_filesToUpload;
    ExtendedState m_extendedState;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODIRECTDEVICEUPLOADSTEP_H
