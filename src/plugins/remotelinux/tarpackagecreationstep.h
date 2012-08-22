/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
#ifndef TARPACKAGECREATIONSTEP_H
#define TARPACKAGECREATIONSTEP_H

#include "abstractpackagingstep.h"
#include "remotelinux_export.h"

#include <projectexplorer/deployablefile.h>

QT_BEGIN_NAMESPACE
class QFile;
class QFileInfo;
QT_END_NAMESPACE

namespace RemoteLinux {

class REMOTELINUX_EXPORT TarPackageCreationStep : public AbstractPackagingStep
{
    Q_OBJECT
public:
    TarPackageCreationStep(ProjectExplorer::BuildStepList *bsl);
    TarPackageCreationStep(ProjectExplorer::BuildStepList *bsl, TarPackageCreationStep *other);

    static Core::Id stepId();
    static QString displayName();

    bool init();
    void run(QFutureInterface<bool> &fi);
private:
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    QString packageFileName() const;

    void ctor();
    bool doPackage(QFutureInterface<bool> &fi);
    bool appendFile(QFile &tarFile, const QFileInfo &fileInfo,
        const QString &remoteFilePath, const QFutureInterface<bool> &fi);
    bool writeHeader(QFile &tarFile, const QFileInfo &fileInfo,
        const QString &remoteFilePath);

    bool m_packagingNeeded;
    QList<ProjectExplorer::DeployableFile> m_files;
};

} // namespace RemoteLinux

#endif // TARPACKAGECREATIONSTEP_H
