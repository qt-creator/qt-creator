// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericdirectuploadstep.h"

#include "genericdirectuploadservice.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfigurationaspects.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

GenericDirectUploadStep::GenericDirectUploadStep(BuildStepList *bsl, Utils::Id id,
                                                 bool offerIncrementalDeployment)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    auto service = new GenericDirectUploadService;
    setDeployService(service);

    BoolAspect *incremental = nullptr;
    if (offerIncrementalDeployment) {
        incremental = addAspect<BoolAspect>();
        incremental->setSettingsKey("RemoteLinux.GenericDirectUploadStep.Incremental");
        incremental->setLabel(Tr::tr("Incremental deployment"),
                              BoolAspect::LabelPlacement::AtCheckBox);
        incremental->setValue(true);
        incremental->setDefaultValue(true);
    }

    auto ignoreMissingFiles = addAspect<BoolAspect>();
    ignoreMissingFiles->setSettingsKey("RemoteLinux.GenericDirectUploadStep.IgnoreMissingFiles");
    ignoreMissingFiles->setLabel(Tr::tr("Ignore missing files"),
                                 BoolAspect::LabelPlacement::AtCheckBox);
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
}

GenericDirectUploadStep::~GenericDirectUploadStep() = default;

Utils::Id GenericDirectUploadStep::stepId()
{
    return Constants::DirectUploadStepId;
}

QString GenericDirectUploadStep::displayName()
{
    return Tr::tr("Upload files via SFTP");
}

} //namespace RemoteLinux
