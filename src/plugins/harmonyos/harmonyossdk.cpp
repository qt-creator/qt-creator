// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyossdk.h"
#include "harmonyosconstants.h"
#include "harmonyostr.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>

#include <coreplugin/icore.h>

#include <utils/elfreader.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>

using namespace Utils;

namespace HarmonyOs::Internal::Sdk {

Environment hdcEnvironment()
{
    Environment env = Environment::systemEnvironment();
    env.set("TMPDIR", TemporaryDirectory::masterDirectoryPath());
    return env;
}

// Where the compiler and its utilities sit. The SDK keeps them in an "llvm" folder, the
// native package Qt Creator carries on the device has one flat "bin" instead.
static FilePath toolchainBinPath(const FilePath &native)
{
    const FilePath llvm = native.pathAppended("llvm/bin");
    return llvm.isDir() ? llvm : native.pathAppended("bin");
}

FilePath nativeSdkPath(const FilePath &sdkRoot)
{
    if (sdkRoot.isEmpty())
        return {};

    // Candidate locations of the "openharmony/native" folder, relative to the configured
    // SDK root. Covers a DevEco Studio installation ("<root>/sdk/..."), a command-line-tools
    // installation ("<root>/sdk/...") and pointing directly at the "sdk" or "native" folder.
    static const QStringList candidates = {
        "sdk/default/openharmony/native",
        "default/openharmony/native",
        "openharmony/native",
        "native",
        "",
    };

    for (const QString &candidate : candidates) {
        const FilePath native = candidate.isEmpty() ? sdkRoot : sdkRoot.pathAppended(candidate);
        if (native.pathAppended("sysroot").isDir() && toolchainBinPath(native).isDir())
            return native;
    }
    return {};
}

FilePath clangCompiler(const FilePath &sdkRoot, bool cxx)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    const QString compiler = cxx ? QString("clang++") : QString("clang");
    return toolchainBinPath(native).pathAppended(compiler).withExecutableSuffix();
}

FilePath lldbCommand(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    return toolchainBinPath(native).pathAppended("lldb").withExecutableSuffix();
}

FilePath cmakeToolchainFile(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    const FilePath toolchainFile = native.pathAppended("build/cmake/ohos.toolchain.cmake");
    return toolchainFile.exists() ? toolchainFile : FilePath();
}

FilePath hdcCommand(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    // hdc lives in the "toolchains" folder next to the "native" folder.
    const FilePath hdc = native.parentDir().pathAppended("toolchains/hdc").withExecutableSuffix();
    return hdc.exists() ? hdc : FilePath();
}

FilePath hapSignToolJar(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    const FilePath jar = native.parentDir().pathAppended("toolchains/lib/hap-sign-tool.jar");
    return jar.exists() ? jar : FilePath();
}

FilePath binarySignTool(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    const FilePath tool = native.parentDir().pathAppended("toolchains/lib/binary-sign-tool");
    return tool.exists() ? tool : FilePath();
}

QString sdkVersion(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    // Any of the SDK components carries the version the SDK manager knows it by.
    const FilePath package = native.parentDir().pathAppended("js/oh-uni-package.json");
    const Result<QByteArray> contents = package.fileContents();
    if (!contents)
        return {};
    const QJsonObject json = QJsonDocument::fromJson(*contents).object();
    const QString version = json.value("version").toString();
    const QString api = json.value("apiVersion").toString();
    if (version.isEmpty() || api.isEmpty())
        return {};
    // "6.0.2.130" is spelled "6.0.2" where a compatible version is asked for.
    const QStringList parts = version.split('.');
    if (parts.size() < 3)
        return {};
    return QString("%1.%2.%3(%4)").arg(parts.at(0), parts.at(1), parts.at(2), api);
}

QStringList restrictedPermissions(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    const FilePath definitions
        = native.parentDir().pathAppended("toolchains/lib/PermissionDefinitions.json");
    const Result<QByteArray> contents = definitions.fileContents();
    if (!contents)
        return {};

    // Anything above "normal" is only granted when the provisioning profile allows it.
    QStringList result;
    const QJsonArray permissions
        = QJsonDocument::fromJson(*contents).object().value("definePermissions").toArray();
    for (const QJsonValue &permission : permissions) {
        const QJsonObject object = permission.toObject();
        if (object.value("availableLevel").toString() != "normal")
            result.append(object.value("name").toString());
    }
    return result;
}

FilePath hnpcliCommand(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    const FilePath tool = native.parentDir().pathAppended("toolchains/hnpcli");
    return tool.exists() ? tool : FilePath();
}

FilePath lldbServerForDevice(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    // The device-side server sits with the target-specific runtime bits, whose directory
    // carries the clang version.
    const FilePath clang = native.pathAppended("llvm/lib/clang");
    for (const FilePath &version : clang.dirEntries(DirFilterFlag::Dirs | DirFilterFlag::NoDotAndDotDot)) {
        const FilePath server = version.pathAppended("bin/aarch64-linux-ohos/lldb-server");
        if (server.exists())
            return server;
    }
    return {};
}

FilePath sysrootPath(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    const FilePath sysroot = native.pathAppended("sysroot");
    return sysroot.isDir() ? sysroot : FilePath();
}

FilePath waitLibrary(const FilePath &sdkRoot)
{
    const FilePath source = Core::ICore::resourcePath("harmonyos/qtcwait.cpp");
    const FilePath compiler = clangCompiler(sdkRoot, true);
    const FilePath sysroot = sysrootPath(sdkRoot);
    if (!source.exists() || compiler.isEmpty() || sysroot.isEmpty())
        return {};

    const FilePath library = Core::ICore::userResourcePath("harmonyos")
                                 .pathAppended(Constants::HARMONYOS_WAIT_LIBRARY);
    if (library.exists() && library.lastModified() > source.lastModified())
        return library;
    if (const Result<> created = library.parentDir().ensureWritableDir(); !created)
        return {};

    Process compile;
    compile.setCommand(
        {compiler,
         {"--target=aarch64-linux-ohos",
          "--sysroot=" + sysroot.path(),
          "-fPIC", "-shared", "-O1",
          // The loader looks for the dependency under this name; without it the linker
          // would record the path the application happened to be linked against.
          "-Wl,-soname," + library.fileName(),
          QString("-DQTC_GATE_PORT=%1").arg(Constants::HARMONYOS_GATE_PORT),
          QString("-DQTC_SERVER_PORT=%1").arg(Constants::HARMONYOS_DEBUG_PORT),
          QString("-DQTC_SERVER_PATH=\"%1\"").arg(Constants::HARMONYOS_DEBUG_SERVER_PATH),
          source.path(), "-o", library.path()}});
    compile.runBlocking();
    return library.exists() ? library : FilePath();
}

FilePath runnerLibrary(const FilePath &sdkRoot)
{
    const FilePath loader = Core::ICore::resourcePath("harmonyos/qtcload.cpp");
    const FilePath source = Core::ICore::resourcePath("harmonyos/qtcrunner.cpp");
    const FilePath compiler = clangCompiler(sdkRoot, true);
    const FilePath sysroot = sysrootPath(sdkRoot);
    if (!loader.exists() || !source.exists() || compiler.isEmpty() || sysroot.isEmpty())
        return {};

    const FilePath library = Core::ICore::userResourcePath("harmonyos")
                                 .pathAppended(Constants::HARMONYOS_RUNNER_LIBRARY);
    const QDateTime newest = std::max(loader.lastModified(), source.lastModified());
    if (library.exists() && library.lastModified() > newest)
        return library;
    if (const Result<> created = library.parentDir().ensureWritableDir(); !created)
        return {};

    Process compile;
    compile.setCommand(
        {compiler,
         {"--target=aarch64-linux-ohos",
          "--sysroot=" + sysroot.path(),
          "-fPIC", "-shared", "-O1", "-std=c++17",
          // The template dlopens the application by this name and calls its main().
          "-Wl,-soname," + library.fileName(),
          "-I" + source.parentDir().path(),
          QString("-DQTC_CHANNEL_PORT=%1").arg(Constants::HARMONYOS_CHANNEL_PORT),
          loader.path(), source.path(), "-o", library.path()}});
    compile.runBlocking();
    return library.exists() ? library : FilePath();
}

// The single directory under "llvm/lib/clang", whose name is the compiler's version and
// part of the layout a clang installation is found by.
static QString clangVersion(const FilePath &native)
{
    const FilePaths versions = native.pathAppended("llvm/lib/clang")
                                   .dirEntries(DirFilterFlag::Dirs
                                               | DirFilterFlag::NoDotAndDotDot);
    return versions.isEmpty() ? QString() : versions.first().fileName();
}

// The SDK's CMake keeps its modules in a directory named after its version.
static QString cmakeShareDirectory(const FilePath &native)
{
    const FilePaths shares = native.pathAppended("build-tools/cmake/share")
                                 .dirEntries(FileFilter({"cmake-*"},
                                                        DirFilterFlag::Dirs
                                                            | DirFilterFlag::NoDotAndDotDot));
    return shares.isEmpty() ? QString() : shares.first().fileName();
}

Result<FilePath> deviceToolchainPackage(const FilePath &deviceSdkRoot, const FilePath &tree,
                                        const std::function<bool()> &isCanceled)
{
    const FilePath native = nativeSdkPath(deviceSdkRoot);
    if (native.isEmpty()) {
        return ResultError(Tr::tr("\"%1\" does not hold an OpenHarmony native SDK.")
                               .arg(deviceSdkRoot.toUserOutput()));
    }
    const FilePath llvm = native.pathAppended("llvm");
    const FilePath clang = llvm.pathAppended("bin/clang");
    if (!clang.isFile())
        return ResultError(Tr::tr("\"%1\" is not in the SDK.").arg(clang.toUserOutput()));
    if (ElfReader(clang).readHeaders().elfmachine != Elf_EM_AARCH64) {
        return ResultError(Tr::tr("The compiler in \"%1\" does not run on a device. What "
                                  "belongs here is the SDK published as \"native-ohos-x64\", "
                                  "whose payload is for arm64 despite the name.")
                               .arg(deviceSdkRoot.toUserOutput()));
    }
    const QString version = clangVersion(native);
    const QString cmakeShare = cmakeShareDirectory(native);
    if (version.isEmpty() || cmakeShare.isEmpty()) {
        return ResultError(Tr::tr("\"%1\" has no compiler runtime or no CMake to take into "
                                  "the package.").arg(deviceSdkRoot.toUserOutput()));
    }

    // What the tree was laid out from, so that pointing at another SDK lays it out again.
    // The leading number is the revision of the layout itself.
    const FilePath stamp = tree.pathAppended("qtctools.stamp");
    const QByteArray state = "1 " + native.toFSPathString().toUtf8() + " "
        + native.pathAppended("oh-uni-package.json").fileContents().value_or(QByteArray());
    if (const Result<QByteArray> current = stamp.fileContents(); current && *current == state)
        return tree;

    if (const Result<> removed = tree.removeRecursively(); !removed)
        return ResultError(removed.error());

    const auto copy = [&isCanceled](const FilePath &from, const FilePath &to) -> Result<> {
        if (!from.exists())
            return ResultError(Tr::tr("\"%1\" is not in the SDK.").arg(from.toUserOutput()));
        if (const Result<> created = to.parentDir().ensureWritableDir(); !created)
            return created;
        if (!from.isDir())
            return from.copyFile(to);
        // File by file, so that a cancel is seen within the sysroot, too.
        const Result<FileUtils::CopyResult> copied = FileUtils::copyRecursively(
            from, to,
            [&isCanceled](const FilePath &source,
                          const FilePath &target) -> Result<FileUtils::CopyResult> {
                if (isCanceled && isCanceled())
                    return FileUtils::CopyResult::Canceled;
                if (const Result<> done = source.copyFile(target); !done)
                    return ResultError(done.error());
                return FileUtils::CopyResult::Done;
            });
        if (!copied)
            return ResultError(copied.error());
        if (*copied == FileUtils::CopyResult::Canceled)
            return ResultError(Tr::tr("Canceled."));
        return ResultOk;
    };

    const QString runtime = QString("lib/clang/%1/lib/aarch64-linux-ohos").arg(version);
    const QList<QPair<FilePath, FilePath>> contents = {
        // The driver picks its language from the name it was called by, and a native
        // package has one flat "bin", so both names are the same binary twice.
        {llvm.pathAppended("bin/clang"), tree.pathAppended("bin/clang")},
        {llvm.pathAppended("bin/clang"), tree.pathAppended("bin/clang++")},
        {llvm.pathAppended("bin/ld.lld"), tree.pathAppended("bin/ld.lld")},
        {llvm.pathAppended("bin/llvm-ar"), tree.pathAppended("bin/llvm-ar")},
        {llvm.pathAppended("bin/llvm-ranlib"), tree.pathAppended("bin/llvm-ranlib")},
        {llvm.pathAppended("bin/llvm-strip"), tree.pathAppended("bin/llvm-strip")},
        {native.pathAppended("build-tools/cmake/bin/cmake"), tree.pathAppended("bin/cmake")},
        {native.pathAppended("build-tools/cmake/bin/ninja"), tree.pathAppended("bin/ninja")},
        // ld.lld reads linker scripts with libxml2, and CMake and ninja were built against
        // the C++ library the SDK carries rather than the older one the device has.
        {llvm.pathAppended("lib/libxml2.so.16"), tree.pathAppended("lib/libxml2.so.16")},
        {llvm.pathAppended("lib/aarch64-linux-ohos/libc++_shared.so"),
         tree.pathAppended("lib/libc++_shared.so")},
        {llvm.pathAppended("include/libcxx-ohos"), tree.pathAppended("include/libcxx-ohos")},
        {llvm.pathAppended("lib/clang/" + version + "/include"),
         tree.pathAppended("lib/clang/" + version + "/include")},
        // Only the compiler runtime, not the sanitizers, which are 40 MB nobody asked for.
        {llvm.pathAppended(runtime + "/libclang_rt.builtins.a"),
         tree.pathAppended(runtime + "/libclang_rt.builtins.a")},
        {llvm.pathAppended(runtime + "/clang_rt.crtbegin.o"),
         tree.pathAppended(runtime + "/clang_rt.crtbegin.o")},
        {llvm.pathAppended(runtime + "/clang_rt.crtend.o"),
         tree.pathAppended(runtime + "/clang_rt.crtend.o")},
        {llvm.pathAppended("lib/aarch64-linux-ohos"),
         tree.pathAppended("lib/aarch64-linux-ohos")},
        {native.pathAppended("build-tools/cmake/share/" + cmakeShare),
         tree.pathAppended("share/" + cmakeShare)},
        {native.pathAppended("sysroot"), tree.pathAppended("sysroot")},
        {native.pathAppended("build/cmake/sdk_native_platforms.cmake"),
         tree.pathAppended("build/cmake/sdk_native_platforms.cmake")},
        {native.pathAppended("oh-uni-package.json"), tree.pathAppended("oh-uni-package.json")},
    };
    for (const QPair<FilePath, FilePath> &item : contents) {
        if (isCanceled && isCanceled())
            return ResultError(Tr::tr("Canceled."));
        if (const Result<> copied = copy(item.first, item.second); !copied)
            return ResultError(copied.error());
    }

    // The SDK's own toolchain file needs no adaptation beyond the two lines that assume the
    // "llvm" directory it normally sits next to: the rest it derives from where it is read
    // from, and a flat "bin" is all the difference.
    const FilePath toolchainFile = native.pathAppended("build/cmake/ohos.toolchain.cmake");
    const Result<QByteArray> chainload = toolchainFile.fileContents();
    if (!chainload)
        return ResultError(chainload.error());
    QByteArray adapted = *chainload;
    adapted.replace("\"${OHOS_SDK_NATIVE}/llvm/bin\"", "\"${OHOS_SDK_NATIVE}/bin\"");
    adapted.replace("\"${OHOS_SDK_NATIVE}/llvm\"", "\"${OHOS_SDK_NATIVE}\"");
    const FilePath adaptedFile = tree.pathAppended("build/cmake/ohos.toolchain.cmake");
    if (const Result<qint64> written = adaptedFile.writeFileContents(adapted); !written)
        return ResultError(written.error());

    // In the application's context the package's files belong to someone else, and only
    // what everyone may execute can be run at all.
    const QFile::Permissions executable = QFile::ReadOwner | QFile::WriteOwner
                                          | QFile::ExeOwner | QFile::ReadGroup
                                          | QFile::ExeGroup | QFile::ReadOther
                                          | QFile::ExeOther;
    for (const QString &directory : {QString("bin"), QString("lib")}) {
        const FilePaths files = tree.pathAppended(directory)
                                    .dirEntries(DirFilterFlag::Files);
        for (const FilePath &file : files) {
            if (const Result<> set = file.setPermissions(executable); !set)
                return ResultError(set.error());
        }
    }

    if (const Result<qint64> written = stamp.writeFileContents(state); !written)
        return ResultError(written.error());
    return tree;
}

FilePath hvigorBinPath(const FilePath &sdkRoot)
{
    if (sdkRoot.isEmpty())
        return {};

    // DevEco Studio keeps hvigor under "tools/hvigor/bin"; the standalone command-line-tools
    // package keeps the hvigorw launcher directly in "bin". Both sit next to the "sdk" folder,
    // which a configured root may point at directly.
    static const QStringList candidates = {"tools/hvigor/bin", "bin"};
    for (const FilePath &root : {sdkRoot, sdkRoot.parentDir()}) {
        for (const QString &candidate : candidates) {
            const FilePath binDir = root.pathAppended(candidate);
            if (binDir.pathAppended("hvigorw").withExecutableSuffix().exists()
                || binDir.pathAppended("hvigorw.bat").exists()) {
                return binDir;
            }
        }
    }
    return {};
}

FilePath hvigorCommand(const FilePath &sdkRoot)
{
    const FilePath bin = hvigorBinPath(sdkRoot);
    if (bin.isEmpty())
        return {};
    // harmonydeployqt runs the launcher directly, so hand it the platform variant.
    const FilePath launcher = HostOsInfo::isWindowsHost() ? bin.pathAppended("hvigorw.bat")
                                                          : bin.pathAppended("hvigorw");
    return launcher.exists() ? launcher : FilePath();
}

FilePath nodeBinPath(const FilePath &sdkRoot)
{
    if (sdkRoot.isEmpty())
        return {};

    static const QStringList candidates = {"tools/node", "node"};
    for (const FilePath &root : {sdkRoot, sdkRoot.parentDir()}) {
        for (const QString &candidate : candidates) {
            const FilePath nodeDir = root.pathAppended(candidate);
            if (nodeDir.pathAppended("node").withExecutableSuffix().exists())
                return nodeDir;
        }
    }
    return {};
}

FilePath devEcoSdkHome(const FilePath &sdkRoot)
{
    if (sdkRoot.isEmpty())
        return {};
    const FilePath sdk = sdkRoot.pathAppended("sdk");
    return sdk.isDir() ? sdk : sdkRoot;
}

bool isValidSdk(const FilePath &sdkRoot)
{
    const FilePath compiler = clangCompiler(sdkRoot, /*cxx=*/true);
    return !compiler.isEmpty() && compiler.exists();
}

void addToEnvironment(const FilePath &sdkRoot, Environment &env)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (!native.isEmpty())
        env.set(Constants::NATIVE_OHOS_SDK_ENV_VAR, native.toUserOutput());

    const FilePath devEco = devEcoSdkHome(sdkRoot);
    if (!devEco.isEmpty())
        env.set(Constants::DEVECO_SDK_HOME_ENV_VAR, devEco.toUserOutput());

    const FilePath hvigor = hvigorBinPath(sdkRoot);
    if (!hvigor.isEmpty())
        env.prependOrSetPath(hvigor);

    // harmonydeployqt does not search PATH for hvigor; it needs this env var.
    const FilePath hvigorw = hvigorCommand(sdkRoot);
    if (!hvigorw.isEmpty())
        env.set(Constants::HVIGOR_ENV_VAR, hvigorw.toUserOutput());

    const FilePath node = nodeBinPath(sdkRoot);
    if (!node.isEmpty())
        env.prependOrSetPath(node);
}

FilePath detectDevEcoSdk()
{
    FilePaths candidates;

    // An explicitly configured SDK home wins. It points at the "sdk" folder, so the
    // installation root that also holds the build tools is its parent.
    const QString envSdkHome = qtcEnvironmentVariable(Constants::DEVECO_SDK_HOME_ENV_VAR);
    if (!envSdkHome.isEmpty()) {
        const FilePath sdkHome = FilePath::fromUserInput(envSdkHome);
        candidates << sdkHome.parentDir() << sdkHome;
    }

    if (HostOsInfo::isWindowsHost()) {
        const QString localAppData = qtcEnvironmentVariable("LOCALAPPDATA");
        if (!localAppData.isEmpty()) {
            candidates << FilePath::fromUserInput(localAppData)
                              .pathAppended("Huawei/DevEco Studio");
        }
    } else if (HostOsInfo::isMacHost()) {
        candidates << FilePath::fromUserInput("/Applications/DevEco-Studio.app/Contents");
    }

    for (const FilePath &candidate : std::as_const(candidates)) {
        if (isValidSdk(candidate))
            return candidate;
    }
    return {};
}

PublicSdk publicSdk()
{
    // The Linux and the Windows tools travel in one archive, macOS has one per
    // architecture, and the payload of the Apple Silicon one is named after neither.
    QString archive;
    QString hostDirectory;
    if (HostOsInfo::isLinuxHost()) {
        archive = "ohos-sdk-windows_linux-public.tar.gz";
        hostDirectory = "linux";
    } else if (HostOsInfo::isWindowsHost()) {
        archive = "ohos-sdk-windows_linux-public.tar.gz";
        hostDirectory = "windows";
    } else if (HostOsInfo::isMacHost()) {
        archive = QSysInfo::currentCpuArchitecture() == "arm64"
                      ? QString("L2-SDK-MAC-M1-PUBLIC.tar.gz")
                      : QString("ohos-sdk-mac-public.tar.gz");
        hostDirectory = "darwin";
    } else {
        return {};
    }

    const QString url = QString("https://repo.huaweicloud.com/openharmony/os/%1-Release/%2")
                            .arg(Constants::PUBLIC_SDK_VERSION, archive);
    return {QUrl(url), hostDirectory};
}

QStringList publicSdkComponents()
{
    return {"native", "toolchains"};
}

} // namespace HarmonyOs::Internal::Sdk
