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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef MAEMODEPLOYSTEP_H
#define MAEMODEPLOYSTEP_H

#include "abstractmaemodeploystep.h"

namespace RemoteLinux {
namespace Internal {
class AbstractMaemoPackageInstaller;
class MaemoPackageUploader;

class AbstractMaemoUploadAndInstallStep : public AbstractMaemoDeployStep
{
    Q_OBJECT
protected:
    AbstractMaemoUploadAndInstallStep(ProjectExplorer::BuildStepList *bc,
        const QString &id);
    AbstractMaemoUploadAndInstallStep(ProjectExplorer::BuildStepList *bc,
        AbstractMaemoUploadAndInstallStep *other);

    void finishInitialization(const QString &displayName,
        AbstractMaemoPackageInstaller *installer);

private slots:
    void handleUploadFinished(const QString &errorMsg);
    void handleInstallationFinished(const QString &errorMsg);

private:
    enum ExtendedState { Inactive, Uploading, Installing };

    virtual bool isDeploymentPossibleInternal(QString &whynot) const;
    virtual bool isDeploymentNeeded(const QString &hostName) const;
    virtual void startInternal();
    virtual void stopInternal();

    void upload();
    void setFinished();
    QString uploadDir() const;

    MaemoPackageUploader *m_uploader;
    AbstractMaemoPackageInstaller *m_installer;
    ExtendedState m_extendedState;
};


class MaemoUploadAndInstallDpkgPackageStep : public AbstractMaemoUploadAndInstallStep
{
    Q_OBJECT
public:
    MaemoUploadAndInstallDpkgPackageStep(ProjectExplorer::BuildStepList *bc);
    MaemoUploadAndInstallDpkgPackageStep(ProjectExplorer::BuildStepList *bc,
        MaemoUploadAndInstallDpkgPackageStep *other);

    static const QString Id;
    static QString displayName();

private:
    void ctor();

    virtual const AbstractMaemoPackageCreationStep *packagingStep() const;
};

class MaemoUploadAndInstallRpmPackageStep : public AbstractMaemoUploadAndInstallStep
{
    Q_OBJECT
public:
    MaemoUploadAndInstallRpmPackageStep(ProjectExplorer::BuildStepList *bc);
    MaemoUploadAndInstallRpmPackageStep(ProjectExplorer::BuildStepList *bc,
        MaemoUploadAndInstallRpmPackageStep *other);

    static const QString Id;
    static QString displayName();

private:
    void ctor();

    virtual const AbstractMaemoPackageCreationStep *packagingStep() const;
};

class MaemoUploadAndInstallTarPackageStep : public AbstractMaemoUploadAndInstallStep
{
    Q_OBJECT
public:
    MaemoUploadAndInstallTarPackageStep(ProjectExplorer::BuildStepList *bc);
    MaemoUploadAndInstallTarPackageStep(ProjectExplorer::BuildStepList *bc,
        MaemoUploadAndInstallTarPackageStep *other);

    static const QString Id;
    static QString displayName();

private:
    void ctor();

    virtual const AbstractMaemoPackageCreationStep *packagingStep() const;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMODEPLOYSTEP_H
