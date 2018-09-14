/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "genericdirectuploadstep.h"

#include "genericdirectuploadservice.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfigurationaspects.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class GenericDirectUploadStepPrivate
{
public:
    GenericDirectUploadService deployService;
    BaseBoolAspect *incrementalAspect;
    BaseBoolAspect *ignoreMissingFilesAspect;
};

} // namespace Internal

GenericDirectUploadStep::GenericDirectUploadStep(BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    d = new Internal::GenericDirectUploadStepPrivate;

    d->incrementalAspect = addAspect<BaseBoolAspect>();
    d->incrementalAspect->setSettingsKey("RemoteLinux.GenericDirectUploadStep.Incremental");
    d->incrementalAspect->setLabel(tr("Incremental deployment"));
    d->incrementalAspect->setValue(true);
    d->incrementalAspect->setDefaultValue(true);

    d->ignoreMissingFilesAspect = addAspect<BaseBoolAspect>();
    d->ignoreMissingFilesAspect
            ->setSettingsKey("RemoteLinux.GenericDirectUploadStep.IgnoreMissingFiles");
    d->ignoreMissingFilesAspect->setLabel(tr("Ignore missing files"));
    d->ignoreMissingFilesAspect->setValue(false);

    setDefaultDisplayName(displayName());
}

GenericDirectUploadStep::~GenericDirectUploadStep()
{
    delete d;
}

bool GenericDirectUploadStep::initInternal(QString *error)
{
    d->deployService.setDeployableFiles(target()->deploymentData().allFiles());
    d->deployService.setIncrementalDeployment(d->incrementalAspect->value());
    d->deployService.setIgnoreMissingFiles(d->ignoreMissingFilesAspect->value());
    return d->deployService.isDeploymentPossible(error);
}

BuildStepConfigWidget *GenericDirectUploadStep::createConfigWidget()
{
    return BuildStep::createConfigWidget();
}

GenericDirectUploadService *GenericDirectUploadStep::deployService() const
{
    return &d->deployService;
}

Core::Id GenericDirectUploadStep::stepId()
{
    return "RemoteLinux.DirectUploadStep";
}

QString GenericDirectUploadStep::displayName()
{
    return tr("Upload files via SFTP");
}

} //namespace RemoteLinux
