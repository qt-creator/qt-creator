// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosqtversion.h"

#include "harmonyosconfigurations.h"
#include "harmonyosconstants.h"
#include "harmonyossettings.h"
#include "harmonyostr.h"

#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionfactory.h>

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace HarmonyOs::Internal {

HarmonyOsQtVersion::HarmonyOsQtVersion() = default;

QString HarmonyOsQtVersion::description() const
{
    //: Qt Version is meant for HarmonyOS
    return Tr::tr("HarmonyOS");
}

Abis HarmonyOsQtVersion::detectQtAbis() const
{
    Abis abis = QtVersion::detectQtAbis();
    if (abis.isEmpty())
        abis = Abi::abisOfBinary(libraryPath().pathAppended("libQt6Core.so"));

    Abis harmonyOsAbis;
    for (const Abi &abi : abis) {
        if (abi.architecture() != Abi::UnknownArchitecture) {
            harmonyOsAbis.append(Abi(abi.architecture(), Abi::LinuxOS,
                                     Abi::OpenHarmonyLinuxFlavor, Abi::ElfFormat,
                                     abi.wordWidth()));
        }
    }
    return harmonyOsAbis;
}

QSet<Id> HarmonyOsQtVersion::targetDeviceTypes() const
{
    return {Constants::HARMONYOS_DEVICE_TYPE, Constants::HARMONYOS_BUILD_DEVICE_TYPE};
}

QSet<Id> HarmonyOsQtVersion::availableFeatures() const
{
    QSet<Id> features = QtVersion::availableFeatures();
    features.insert(QtSupport::Constants::FEATURE_MOBILE);
    features.remove(QtSupport::Constants::FEATURE_QT_WEBKIT);
    return features;
}

// The device refuses to execute this Qt's own tools, so the copies of qtchost in Qt Creator's
// package that a build runs instead map them, and this is where they find them. The host Qt
// a cross build names is not on the device, the tools built for the device are its own.
void HarmonyOsQtVersion::addToBuildEnvironment(const Kit *k, Environment &env) const
{
    QtVersion::addToBuildEnvironment(k, env);
    if (HostOsInfo::isHarmonyHost()) {
        env.set("QTC_QT_HOST_LIBEXECS", libExecPath().path());
        env.set("QTC_QT_HOST_BINS", binPath().path());
        // The Qt libraries need the third-party ones the application package gets, ICU
        // among them, and the device has other versions of those. Tools that bring a
        // QtCore of their own, built without ICU, get that one.
        QString libraries = libraryPath().path();
        if (const FilePath toolsLibrary = libExecPath().pathAppended("lib");
            toolsLibrary.pathAppended("libQt6Core.so").isFile()) {
            libraries.prepend(toolsLibrary.path() + ':');
        }
        if (const FilePath packages = settings().additionalPackages(); !packages.isEmpty())
            libraries += ':' + packages.pathAppended("lib").path();
        env.set("QTC_QT_HOST_LIBS", libraries);
        // The package is laid out like the native SDK the ohos mkspec expects.
        env.set("NATIVE_OHOS_SDK",
                FilePath::fromString(Constants::HARMONYOS_NATIVE_PACKAGE_BIN).parentDir().path());
        if (const Abis abis = qtAbis(); !abis.isEmpty())
            env.set("OHOS_TARGET_ARCH", ohosAbiName(abis.first()));
    }
}

// The compiler the device builds with is detected as a plain clang, which would choose
// linux-clang over the mkspec this Qt was built for.
QString HarmonyOsQtVersion::mkspecFor(Toolchain *tc) const
{
    if (HostOsInfo::isHarmonyHost())
        return mkspec();
    return QtVersion::mkspecFor(tc);
}

// A Qt known from its prefix has a qmake all the same, if one the device does not execute
// either. The Makefiles it writes name it, and the copy of qtchost runs it.
FilePath HarmonyOsQtVersion::qmakeFilePath() const
{
    if (HostOsInfo::isHarmonyHost() && hasDataFromFiles())
        return binPath().pathAppended("qmake");
    return QtVersion::qmakeFilePath();
}

CommandLine HarmonyOsQtVersion::qmakeCommand() const
{
    if (!HostOsInfo::isHarmonyHost())
        return QtVersion::qmakeCommand();
    const FilePath packageBin = FilePath::fromString(Constants::HARMONYOS_NATIVE_PACKAGE_BIN);
    CommandLine command{packageBin.pathAppended("qmake")};
    // A Qt built on another host describes its layout there.
    const FilePath targetQtConf = binPath().pathAppended("target_qt.conf");
    if (targetQtConf.exists())
        command.addArgs({"-qtconf", targetQtConf.path()});
    // qmake would name the tools in this Qt, or those of the host a Qt was built on.
    for (const QString &tool : QStringList{"moc", "rcc", "uic", "qlalr"})
        command.addArg("QT_TOOL." + tool + ".binary=" + packageBin.pathAppended(tool).path());
    // The mkspec looks for the compiler in the llvm/bin of the SDK, the package has one bin.
    // -early, so that the compiler check in default_pre.prf already sees these.
    const QString cc = packageBin.pathAppended("clang").path();
    const QString cxx = packageBin.pathAppended("clang++").path();
    command.addArgs({"-early",
                     "QMAKE_CC=" + cc, "QMAKE_LINK_C=" + cc, "QMAKE_LINK_C_SHLIB=" + cc,
                     "QMAKE_CXX=" + cxx, "QMAKE_LINK=" + cxx, "QMAKE_LINK_SHLIB=" + cxx,
                     "QMAKE_AR=" + packageBin.pathAppended("llvm-ar").path() + " cqs",
                     "QMAKE_RANLIB=" + packageBin.pathAppended("llvm-ranlib").path(),
                     "QMAKE_STRIP=" + packageBin.pathAppended("llvm-strip").path(),
                     "-before"});
    return command;
}

class HarmonyOsQtVersionFactory final : public QtVersionFactory
{
public:
    HarmonyOsQtVersionFactory()
    {
        setQtVersionCreator([] { return new HarmonyOsQtVersion; });
        setSupportedType(Constants::HARMONYOS_QT_TYPE);
        setPriority(90);
        setRestrictionChecker([](const SetupData &setup) {
            // The ohos mkspec bails out unless NATIVE_OHOS_SDK is set in the environment,
            // which leaves QMAKE_PLATFORM empty. The mkspec name is read from qmake's
            // QMAKE_XSPEC and stays available, so fall back to it.
            return setup.platforms.contains("ohos") || setup.mkspec.startsWith("ohos-");
        });
    }
};

void setupHarmonyOsQtVersion()
{
    static HarmonyOsQtVersionFactory theHarmonyOsQtVersionFactory;
}

} // namespace HarmonyOs::Internal
