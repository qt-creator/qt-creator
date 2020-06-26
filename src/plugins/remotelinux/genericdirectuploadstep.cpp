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
#include "remotelinux_constants.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfigurationaspects.h>

using namespace ProjectExplorer;

namespace RemoteLinux {

GenericDirectUploadStep::GenericDirectUploadStep(BuildStepList *bsl, Utils::Id id,
                                                 bool offerIncrementalDeployment)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    auto service = createDeployService<GenericDirectUploadService>();

    BaseBoolAspect *incremental = nullptr;
    if (offerIncrementalDeployment) {
        incremental = addAspect<BaseBoolAspect>();
        incremental->setSettingsKey("RemoteLinux.GenericDirectUploadStep.Incremental");
        incremental->setLabel(tr("Incremental deployment"),
                              BaseBoolAspect::LabelPlacement::AtCheckBox);
        incremental->setValue(true);
        incremental->setDefaultValue(true);
    }

    auto ignoreMissingFiles = addAspect<BaseBoolAspect>();
    ignoreMissingFiles->setSettingsKey("RemoteLinux.GenericDirectUploadStep.IgnoreMissingFiles");
    ignoreMissingFiles->setLabel(tr("Ignore missing files"),
                                 BaseBoolAspect::LabelPlacement::AtCheckBox);
    ignoreMissingFiles->setValue(false);

    setInternalInitializer([incremental, ignoreMissingFiles, service] {
        if (incremental) {
            service->setIncrementalDeployment(incremental->value()
                ? IncrementalDeployment::Enabled : IncrementalDeployment::Disabled);
        } else {
            service->setIncrementalDeployment(IncrementalDeployment::NotSupported);
        }
        service->setIgnoreMissingFiles(ignoreMissingFiles->value());
        return service->isDeploymentPossible();
    });

    setRunPreparer([this, service] {
        service->setDeployableFiles(target()->deploymentData().allFiles());
    });

    setDefaultDisplayName(displayName());
}

GenericDirectUploadStep::~GenericDirectUploadStep() = default;

Utils::Id GenericDirectUploadStep::stepId()
{
    return Constants::DirectUploadStepId;
}

QString GenericDirectUploadStep::displayName()
{
    return tr("Upload files via SFTP");
}

} //namespace RemoteLinux
