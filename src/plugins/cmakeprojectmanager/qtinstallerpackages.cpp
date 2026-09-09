// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtinstallerpackages.h"

#include <utils/algorithm.h>

#include <QHash>
#include <QVersionNumber>

namespace CMakeProjectManager::Internal {

QString qtInstallerPlatform(const QString &buildFlavor, bool crossCompiled)
{
    if (buildFlavor == "msvc2022_arm64" && crossCompiled)
        return "win64_msvc2022_arm64_cross_compiled";

    static const QHash<QString, QString> platforms = {
        {"llvm-mingw_64", "win64_llvm_mingw"},
        {"mingw_64", "win64_mingw"},
        {"msvc2019_64", "win64_msvc2019_64"},
        {"msvc2022_64", "win64_msvc2022_64"},
        {"msvc2022_arm64", "win64_msvc2022_arm64"},
        {"gcc_64", "linux_gcc_64"},
        {"gcc_arm64", "linux_gcc_arm64"},
        {"ios", "ios"},
        {"macos", "clang_64"},
        {"android_arm64_v8a", "android"},
        {"android_armv7", "android"},
        {"android_x86", "android"},
        {"android_x86_64", "android"},
        {"wasm_multithread", "wasm_multithread"},
        {"wasm_singlethread", "wasm_singlethread"}};

    return platforms.value(buildFlavor);
}

static QStringList addonPackages()
{
    return {"qt3d",
            "qt5compat",
            "qtcanvaspainter",
            "qtcharts",
            "qtconnectivity",
            "qtcoap",
            "qtdatavis3d",
            "qtgraphs",
            "qtgrpc",
            "qthttpserver",
            "qtimageformats",
            "qtlocation",
            "qtlottie",
            "qtmultimedia",
            "qtnetworkauth",
            "qtpositioning",
            "qtquick3d",
            "qtquick3dphysics",
            "qtquickeffectmaker",
            "qtquicktimeline",
            "qtopenapi",
            "qtremoteobjects",
            "qtscxml",
            "qtsensors",
            "qtserialbus",
            "qtserialport",
            "qtshadertools",
            "qtspeech",
            "qttasktree",
            "qtvirtualkeyboard",
            "qtwebchannel",
            "qtwebsockets",
            "qtwebview",

            // found in commercial version
            "qtapplicationmanager",
            "qtinterfaceframework",
            "qtlanguageserver",
            "qtmqtt",
            "qtstatemachine",
            "qtopcua",
            "tqtc-qtvncserver"};
}

static QStringList extensionPackages()
{
    return {"qtinsighttracker", "qtpdf", "qtwebengine"};
}

// Addons that come as a package of their own, and not below the addons group.
static QStringList standaloneAddonPackages()
{
    return {"qtquick3d", "qt5compat", "qtshadertools", "qtquicktimeline"};
}

// Addons of a single platform, which the group of a component only holds there.
static QStringList platformAddonPackages(const QString &platform)
{
    if (platform.startsWith("win64"))
        return {"qtactiveqt"};
    if (platform.startsWith("linux") || platform == "android")
        return {"qtwaylandcompositor"};
    return {};
}

// The modules whose name does not start with the name of the package that
// ships them, and the package that does. A component names the module it
// belongs to, and the private and the tools part of a module extend its name,
// so the longest module name a component starts with is the one that answers
// for it: ProtobufQtCoreTypes and ProtobufTools come with qtgrpc, as Protobuf
// does.
static QString aliasedPackage(const QString &component)
{
    static const QHash<QString, QString> packages = {
        {"axbase", "qtactiveqt"},
        {"axcontainer", "qtactiveqt"},
        {"axserver", "qtactiveqt"},
        {"bluetooth", "qtconnectivity"},
        {"core5compat", "qt5compat"},
        {"datavisualization", "qtdatavis3d"},
        {"ffmpegmediapluginimpl", "qtmultimedia"},
        {"jsonrpc", "qtlanguageserver"},
        {"nfc", "qtconnectivity"},
        {"protobuf", "qtgrpc"},
        {"quick3dspatialaudio", "qtmultimedia"},
        {"repparser", "qtremoteobjects"},
        {"spatialaudio", "qtmultimedia"},
        {"texttospeech", "qtspeech"},
        {"vncserver", "tqtc-qtvncserver"},
        {"waylandclient", "qtwaylandcompositor"}};

    QString module;
    for (auto it = packages.cbegin(); it != packages.cend(); ++it) {
        if (component.startsWith(it.key()) && it.key().size() > module.size())
            module = it.key();
    }
    return packages.value(module);
}

// A component belongs to the package whose name its own extends, the longest
// one of those: Quick3DPhysics comes with qtquick3dphysics, and not with
// qtquick3d.
static QString packageOf(const QString &component, const QStringList &packages)
{
    const QString name = component.toLower();
    if (const QString alias = aliasedPackage(name); packages.contains(alias))
        return alias;

    const QString moduleName = "qt" + name;
    QString result;
    for (const QString &package : packages) {
        if (moduleName.startsWith(package) && package.size() > result.size())
            result = package;
    }
    return result;
}

QStringList qtInstallerPackages(const QStringList &components,
                                const QString &qtVersion,
                                const QString &platform)
{
    const QVersionNumber version = QVersionNumber::fromString(qtVersion);
    const QString majorVersion = QString::number(version.majorVersion());
    const QString dotlessVersion = QString(qtVersion).remove('.');

    QStringList addons = addonPackages() + platformAddonPackages(platform);
    QStringList standaloneAddons;

    // Up to 6.8.0, the extensions were addons, and the standalone addons had
    // no group of their own either.
    if (version < QVersionNumber(6, 8, 0)) {
        standaloneAddons = standaloneAddonPackages();
        addons = Utils::filtered(addons, [&standaloneAddons](const QString &addon) {
            return !standaloneAddons.contains(addon);
        });
        addons += extensionPackages();
    }

    QStringList result;
    for (const QString &component : components) {
        QString package;
        if (const QString addon = packageOf(component, addons); !addon.isEmpty())
            package = QString("qt.qt%1.%2.addons.%3").arg(majorVersion, dotlessVersion, addon);
        else if (const QString extension = packageOf(component, extensionPackages());
                 !extension.isEmpty())
            package = QString("extensions.%1.%2.%3").arg(extension, dotlessVersion, platform);
        else if (const QString standalone = packageOf(component, standaloneAddons);
                 !standalone.isEmpty())
            package = QString("qt.qt%1.%2.%3").arg(majorVersion, dotlessVersion, standalone);
        else // The Desktop package holds everything else.
            package = QString("qt.qt%1.%2.%3").arg(majorVersion, dotlessVersion, platform);

        if (!result.contains(package))
            result.append(package);
    }
    return result;
}

} // CMakeProjectManager::Internal
