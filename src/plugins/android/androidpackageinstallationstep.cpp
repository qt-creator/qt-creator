/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidpackageinstallationstep.h"
#include "androidtarget.h"

#include <QFileInfo>
#include <QDir>
#include <projectexplorer/buildsteplist.h>

using namespace Android::Internal;

const Core::Id AndroidPackageInstallationStep::Id("Qt4ProjectManager.AndroidPackageInstallationStep");

static inline QString stepDisplayName()
{
    return AndroidPackageInstallationStep::tr("Copy application data");
}

AndroidPackageInstallationStep::AndroidPackageInstallationStep(ProjectExplorer::BuildStepList *bsl) : MakeStep(bsl, Id)
{
    const QString name = stepDisplayName();
    setDefaultDisplayName(name);
    setDisplayName(name);
}

AndroidPackageInstallationStep::AndroidPackageInstallationStep(ProjectExplorer::BuildStepList *bc, AndroidPackageInstallationStep *other): MakeStep(bc, other)
{
    const QString name = stepDisplayName();
    setDefaultDisplayName(name);
    setDisplayName(name);
}

AndroidPackageInstallationStep::~AndroidPackageInstallationStep()
{
}

bool AndroidPackageInstallationStep::init()
{
    AndroidTarget *androidTarget = qobject_cast<AndroidTarget *>(target());
    if (!androidTarget) {
        emit addOutput(tr("Current target is not an Android target"), BuildStep::MessageOutput);
        return false;
    }

    setUserArguments(QString::fromLatin1("INSTALL_ROOT=\"%1\" install").arg(QDir::toNativeSeparators(androidTarget->androidDirPath())));

    return MakeStep::init();
}
