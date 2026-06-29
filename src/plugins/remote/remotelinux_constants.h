// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Remote::Constants {

inline constexpr char GenericLinuxOsType[] = "GenericLinuxOsType";
inline constexpr char GenericWindowsOsType[] = "GenericWindowsOsType";

inline constexpr char DeployToGenericLinux[] = "DeployToGenericLinux";

inline constexpr char ConnectStepId[] = "RemoteLinux.ConnectStep";
inline constexpr char DirectUploadStepId[] = "RemoteLinux.DirectUploadStep";
inline constexpr char MakeInstallStepId[] = "RemoteLinux.MakeInstall";
inline constexpr char TarPackageCreationStepId[]  = "MaemoTarPackageCreationStep";
inline constexpr char TarPackageFilePathId[] = "TarPackageFilePath";
inline constexpr char TarPackageDeployStepId[] = "MaemoUploadAndInstallTarPackageStep";
inline constexpr char GenericDeployStepId[] = "RemoteLinux.RsyncDeployStep";
inline constexpr char CustomCommandDeployStepId[] = "RemoteLinux.GenericRemoteLinuxCustomCommandDeploymentStep";
inline constexpr char KillAppStepId[] = "RemoteLinux.KillAppStep";

inline constexpr char SshForwardPort[] = "RemoteLinux.SshForwardPort";
inline constexpr char DisableSharing[] = "RemoteLinux.DisableSharing";

inline constexpr char RunConfigId[] = "RemoteLinuxRunConfiguration:";
inline constexpr char CustomRunConfigId[] = "RemoteLinux.CustomRunConfig";

inline constexpr char ExecutionType[] = "RemoteLinux.ExecutionType";

} // Remote::Constants
