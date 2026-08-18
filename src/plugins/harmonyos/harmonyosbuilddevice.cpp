// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosbuilddevice.h"

#include "harmonyosconstants.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"
#include "harmonyostr.h"

#include <projectexplorer/devicesupport/sshparameters.h>

#include <remote/sshdevicewizard.h>

#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>

#include <QDialog>

using namespace ProjectExplorer;
using namespace Utils;
using namespace std::chrono_literals;

namespace HarmonyOs::Internal {

HarmonyOsBuildDevice::HarmonyOsBuildDevice()
{
    setType(Constants::HARMONYOS_BUILD_DEVICE_TYPE);
    setDisplayType(Tr::tr("HarmonyOS Build Device"));
    setDefaultDisplayName(Tr::tr("HarmonyOS Build Device"));

    // The toolchains there are ordinary Linux ones, and everything that looks for
    // them expects a Linux device.
    setOsType(OsTypeLinux);

    SshParameters sshParams = sshParameters();
    sshParams.setPort(Constants::HARMONYOS_SSH_PORT);
    setDefaultSshParameters(sshParams);
}

// A binary reaching the device unsigned is refused by its code signing, and one
// signed twice is refused as well, so an already signed binary is left alone.
Result<QByteArray> HarmonyOsBuildDevice::prepareExecutableForUpload(const QByteArray &binary) const
{
    const FilePath signTool = Sdk::binarySignTool(settings().sdkLocation());
    if (signTool.isEmpty()) {
        // Uploading it unsigned would leave a bridge that cannot run, which the
        // caller cannot tell from one that works.
        return ResultError(Tr::tr("No HarmonyOS SDK is configured to sign the bridge with. "
                                  "Set it up in Preferences > SDKs > HarmonyOS."));
    }

    TemporaryDirectory directory("qtc-harmonyos-sign");
    if (!directory.isValid())
        return ResultError(directory.errorString());
    const FilePath unsignedFile = directory.filePath("unsigned");
    const FilePath signedFile = directory.filePath("signed");
    if (const Result<qint64> written = unsignedFile.writeFileContents(binary); !written)
        return ResultError(written.error());

    struct SignToolRun
    {
        bool succeeded = false;
        QString output;
    };
    const auto runSignTool = [signTool](const QStringList &arguments) {
        Process process;
        process.setCommand({signTool, arguments});
        process.runBlocking(30s);
        return SignToolRun{process.result() == ProcessResult::FinishedWithSuccess,
                           process.allOutput()};
    };

    // The marker decides, not the exit status: the tool may well report a missing
    // signature as a failure. But a run that neither found the marker nor
    // succeeded says nothing, and reading that as "already signed" would upload
    // the unsigned binary this function exists to prevent.
    const SignToolRun state = runSignTool({"display-sign", "-inFile", unsignedFile.nativePath()});
    if (!state.output.contains("code signature is not found")) {
        if (!state.succeeded) {
            return ResultError(Tr::tr("Could not tell whether the bridge is signed: %1")
                                   .arg(state.output.trimmed()));
        }
        return binary;
    }

    const SignToolRun signing = runSignTool({"sign", "-selfSign", "1",
                                             "-inFile", unsignedFile.nativePath(),
                                             "-outFile", signedFile.nativePath()});
    if (!signedFile.exists())
        return ResultError(Tr::tr("Failed to sign the bridge: %1").arg(signing.output.trimmed()));
    return signedFile.fileContents();
}

HarmonyOsBuildDeviceFactory::HarmonyOsBuildDeviceFactory()
    : IDeviceFactory(Constants::HARMONYOS_BUILD_DEVICE_TYPE)
{
    setDisplayName(Tr::tr("HarmonyOS Build Device"));
    setQuickCreationAllowed(true);
    setConstructionFunction([] { return HarmonyOsBuildDevice::create(); });
    setCreator([]() -> IDevice::Ptr {
        const HarmonyOsBuildDevice::Ptr device = HarmonyOsBuildDevice::create();
        Remote::SshDeviceWizard wizard(Tr::tr("New HarmonyOS Build Device Configuration Setup"),
                                       IDevice::Ptr(device));
        if (wizard.exec() != QDialog::Accepted)
            return {};
        return device;
    });
}

void setupHarmonyOsBuildDevice()
{
    static HarmonyOsBuildDeviceFactory theHarmonyOsBuildDeviceFactory;
}

} // namespace HarmonyOs::Internal
