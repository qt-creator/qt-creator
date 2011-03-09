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

#ifndef MAEMODEPLOYSTEP_H
#define MAEMODEPLOYSTEP_H

#include "abstractmaemodeploystep.h"

namespace Qt4ProjectManager {
namespace Internal {
class AbstractMaemoPackageInstaller;
class MaemoPackageUploader;

class MaemoUploadAndInstallDeployStep : public AbstractMaemoDeployStep
{
    Q_OBJECT
public:
    MaemoUploadAndInstallDeployStep(ProjectExplorer::BuildStepList *bc);
    MaemoUploadAndInstallDeployStep(ProjectExplorer::BuildStepList *bc,
        MaemoUploadAndInstallDeployStep *other);

    static const QString Id;
    static const QString DisplayName;

private slots:
    void handleUploadFinished(const QString &errorMsg);
    void handleInstallationFinished(const QString &errorMsg);

private:
    enum ExtendedState { Inactive, Uploading, Installing };

    virtual bool isDeploymentPossibleInternal(QString &whynot) const;
    virtual bool isDeploymentNeeded(const QString &hostName) const;
    virtual void startInternal();
    virtual void stopInternal();

    void ctor();
    void upload();
    void setFinished();
    QString uploadDir() const;

    MaemoPackageUploader *m_uploader;
    AbstractMaemoPackageInstaller *m_installer;
    ExtendedState m_extendedState;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEPLOYSTEP_H
