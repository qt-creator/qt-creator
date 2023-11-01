// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace RemoteLinux {
namespace Constants {

const char GenericLinuxOsType[] = "GenericLinuxOsType";

const char DeployToGenericLinux[] = "DeployToGenericLinux";

const char DirectUploadStepId[] = "RemoteLinux.DirectUploadStep";
const char MakeInstallStepId[] = "RemoteLinux.MakeInstall";
const char TarPackageCreationStepId[]  = "MaemoTarPackageCreationStep";
const char TarPackageFilePathId[] = "TarPackageFilePath";
const char TarPackageDeployStepId[] = "MaemoUploadAndInstallTarPackageStep";
const char GenericDeployStepId[] = "RemoteLinux.RsyncDeployStep";
const char CustomCommandDeployStepId[] = "RemoteLinux.GenericRemoteLinuxCustomCommandDeploymentStep";
const char KillAppStepId[] = "RemoteLinux.KillAppStep";

const char SourceProfile[] = "RemoteLinux.SourceProfile";
const char LinkDevice[] = "RemoteLinux.LinkDevice";

const char RunConfigId[] = "RemoteLinuxRunConfiguration:";
const char CustomRunConfigId[] = "RemoteLinux.CustomRunConfig";

} // Constants
} // RemoteLinux
