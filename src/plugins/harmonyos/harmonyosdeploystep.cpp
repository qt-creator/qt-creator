// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosdeploystep.h"

#include "harmonyosconstants.h"
#include "harmonyosdevice.h"
#include "harmonyosrunconfiguration.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"
#include "harmonyostr.h"

#include <cmakeprojectmanager/cmakekitaspect.h>

#include <projectexplorer/abstractprocessstep.h>
#include <coreplugin/icore.h>
#include <qtsupport/qtkitaspect.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>

#include <utils/commandline.h>
#include <utils/qtcprocess.h>
#include <utils/environment.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/store.h>
#include <utils/stringutils.h>

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace Utils;

#ifdef WITH_TESTS
#include <QTemporaryDir>
#include <QTest>
#endif

namespace HarmonyOs::Internal {

// Signing values keyed by the QT_HARMONYOS_SIGNING_* environment variable name.
using SigningConfig = QMap<QString, QString>;

// Returns the balanced {...} object at or after "from", honouring string
// literals so that braces inside strings do not end the object.
static QString balancedBraces(const QString &text, qsizetype from)
{
    const qsizetype open = text.indexOf('{', from);
    if (open < 0)
        return {};
    int depth = 0;
    bool inString = false;
    for (qsizetype i = open; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (inString) {
            if (c == '\\')
                ++i; // Skip the escaped character.
            else if (c == '"')
                inString = false;
        } else if (c == '"') {
            inString = true;
        } else if (c == '{') {
            ++depth;
        } else if (c == '}' && --depth == 0) {
            return text.mid(open, i - open + 1);
        }
    }
    return {};
}

// The signing material configured in Preferences > SDKs > HarmonyOS, for projects that
// were not set up with DevEco's automatic signing. Empty unless all of it is present.
static SigningConfig signingConfigFromSettings(const Environment &env)
{
    const FilePath certificate = settings().signingCertificate();
    const FilePath profile = settings().signingProfile();
    const FilePath keystore = settings().signingKeystore();
    const QString alias = settings().signingKeyAlias();
    if (certificate.isEmpty() || profile.isEmpty() || keystore.isEmpty() || alias.isEmpty())
        return {};

    SigningConfig config;
    config.insert(Constants::SIGNING_CERT_PATH_ENV_VAR, certificate.nativePath());
    config.insert(Constants::SIGNING_PROFILE_ENV_VAR, profile.nativePath());
    config.insert(Constants::SIGNING_STORE_FILE_ENV_VAR, keystore.nativePath());
    config.insert(Constants::SIGNING_KEY_ALIAS_ENV_VAR, alias);
    const auto insertPassword = [&config, &env](const QString &var, const QString &password) {
        if (!password.isEmpty())
            config.insert(var, password);
        else if (const QString fromEnv = env.value(var); !fromEnv.isEmpty())
            config.insert(var, fromEnv);
    };
    insertPassword(Constants::SIGNING_KEY_PASSWORD_ENV_VAR, settings().keyPassword());
    insertPassword(Constants::SIGNING_STORE_PASSWORD_ENV_VAR, settings().storePassword());
    return config;
}

// The profile is a PKCS#7 file whose signed content is plain JSON, so this reads it
// without touching the signature.
class ProvisioningProfile
{
public:
    QString bundleName;
    QStringList allowedAcls;
};

// The signature around the JSON can hold stray braces, so the content is the first
// balanced object that parses into something a profile would say.
static QJsonObject signedContent(const QString &text)
{
    for (qsizetype at = text.indexOf('{'); at >= 0; at = text.indexOf('{', at + 1)) {
        const QString object = balancedBraces(text, at);
        if (object.isEmpty())
            continue;
        const QJsonObject json = QJsonDocument::fromJson(object.toUtf8()).object();
        if (json.contains("bundle-info"))
            return json;
    }
    return {};
}

static ProvisioningProfile readProvisioningProfile(const FilePath &profile)
{
    ProvisioningProfile result;
    const Result<QByteArray> contents = profile.fileContents();
    if (!contents)
        return result;

    const QJsonObject json = signedContent(QString::fromLatin1(*contents));
    result.bundleName = json.value("bundle-info").toObject().value("bundle-name").toString();
    for (const QJsonValue &acl : json.value("acls").toObject().value("allowed-acls").toArray())
        result.allowedAcls.append(acl.toString());
    return result;
}

// The permissions Qt asks for that a device only grants when the profile allows them.
// Falls back to the one Qt is known to ask for when the SDK cannot be read.
static QStringList aclPermissions(const FilePath &sdkRoot)
{
    const QStringList restricted = Sdk::restrictedPermissions(sdkRoot);
    return restricted.isEmpty() ? QStringList{"ohos.permission.READ_PASTEBOARD"} : restricted;
}

// A device refuses to install a package that asks for a permission the provisioning
// profile does not allow.
static Result<QStringList> dropPermissions(const FilePath &moduleJson, const QStringList &names)
{
    const Result<QByteArray> contents = moduleJson.fileContents();
    if (!contents)
        return ResultError(contents.error());

    QString text = QString::fromUtf8(*contents);
    QStringList dropped;
    for (const QString &name : names) {
        const qsizetype at = text.indexOf('"' + name + '"');
        if (at < 0)
            continue;
        // The permission sits in an object of its own, which is what has to go.
        qsizetype start = text.lastIndexOf('{', at);
        if (start < 0)
            continue;
        const QString object = balancedBraces(text, start);
        if (object.isEmpty())
            continue;
        qsizetype end = start + object.size();
        while (end < text.size() && (text.at(end) == ',' || text.at(end).isSpace()))
            ++end;
        text.remove(start, end - start);
        dropped.append(name);
    }
    if (dropped.isEmpty())
        return dropped;
    if (const Result<qint64> written = moduleJson.writeFileContents(text.toUtf8()); !written)
        return ResultError(written.error());
    return dropped;
}

static Result<> addPermission(const FilePath &moduleJson, const QString &name)
{
    const Result<QByteArray> contents = moduleJson.fileContents();
    if (!contents)
        return ResultError(contents.error());

    QString text = QString::fromUtf8(*contents);
    if (text.contains('"' + name + '"'))
        return ResultOk;

    static const QRegularExpression re("\"requestPermissions\"\\s*:\\s*\\[");
    const QRegularExpressionMatch match = re.match(text);
    if (!match.hasMatch())
        return ResultError(Tr::tr("No permission list in \"%1\".").arg(moduleJson.toUserOutput()));

    text.insert(match.capturedEnd(), "\n        { \"name\": \"" + name + "\" },");
    if (const Result<qint64> written = moduleJson.writeFileContents(text.toUtf8()); !written)
        return ResultError(written.error());
    return ResultOk;
}

// A native package is only unpacked on the device when the module declares it.
static Result<> declareHnpPackage(const FilePath &moduleJson, const QString &fileName)
{
    const Result<QByteArray> contents = moduleJson.fileContents();
    if (!contents)
        return ResultError(contents.error());

    QString text = QString::fromUtf8(*contents);
    if (text.contains("\"hnpPackages\""))
        return ResultOk;

    static const QRegularExpression re("\"module\"\\s*:\\s*\\{");
    const QRegularExpressionMatch match = re.match(text);
    if (!match.hasMatch())
        return ResultError(Tr::tr("No module object in \"%1\".").arg(moduleJson.toUserOutput()));

    text.insert(match.capturedEnd(),
                "\n    \"hnpPackages\": [\n"
                "      { \"package\": \"" + fileName + "\", \"type\": \"public\" }\n"
                "    ],");
    if (const Result<qint64> written = moduleJson.writeFileContents(text.toUtf8()); !written)
        return ResultError(written.error());
    return ResultOk;
}

// The generated project asks for the SDK version Qt was built against, which is not
// necessarily the one that is installed, and hvigor refuses what its SDK manager cannot find.
static Result<QString> setSdkVersion(const FilePath &buildProfile, const QString &version)
{
    const Result<QByteArray> contents = buildProfile.fileContents();
    if (!contents)
        return ResultError(contents.error());

    static const QRegularExpression re("(\"compatibleSdkVersion\"\\s*:\\s*\")([^\"]*)(\")");
    QString text = QString::fromUtf8(*contents);
    const QRegularExpressionMatch match = re.match(text);
    if (!match.hasMatch())
        return ResultError(Tr::tr("No SDK version in \"%1\".").arg(buildProfile.toUserOutput()));
    const QString previous = match.captured(2);
    if (previous == version)
        return previous;

    text.replace(match.capturedStart(), match.capturedLength(),
                 match.captured(1) + version + match.captured(3));
    if (const Result<qint64> written = buildProfile.writeFileContents(text.toUtf8()); !written)
        return ResultError(written.error());
    return previous;
}

static Result<QString> setBundleName(const FilePath &appJson, const QString &bundleName)
{
    const Result<QByteArray> contents = appJson.fileContents();
    if (!contents)
        return ResultError(contents.error());

    static const QRegularExpression re("(\"bundleName\"\\s*:\\s*\")([^\"]*)(\")");
    QString text = QString::fromUtf8(*contents);
    const QRegularExpressionMatch match = re.match(text);
    if (!match.hasMatch())
        return ResultError(Tr::tr("No bundle name in \"%1\".").arg(appJson.toUserOutput()));
    const QString previous = match.captured(2);
    if (previous == bundleName)
        return previous;

    text.replace(match.capturedStart(), match.capturedLength(),
                 match.captured(1) + bundleName + match.captured(3));
    if (const Result<qint64> written = appJson.writeFileContents(text.toUtf8()); !written)
        return ResultError(written.error());
    return previous;
}

static FilePath packageDir(const BuildConfiguration *bc)
{
    return bc->buildDirectory().pathAppended("harmonyos-package");
}

static FilePath projectDir(const BuildConfiguration *bc)
{
    return bc->buildDirectory().pathAppended("harmonyos-build");
}

// Builds the .hap via the Qt-generated CMake "<target>_make_hap" target.
class MakeHapStep final : public AbstractProcessStep
{
public:
    MakeHapStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        setDisplayName(Tr::tr("Build HarmonyOS package (.hap)"));
        setSummaryUpdater([this] {
            const QString target = makeHapTarget();
            return Tr::tr("<b>Build HarmonyOS package:</b> %1")
                .arg(target.isEmpty() ? Tr::tr("(no target)") : target);
        });
    }

private:
    QString makeHapTarget() const
    {
        const QString buildKey = buildConfiguration()->activeBuildKey();
        return buildKey.isEmpty() ? QString() : buildKey + "_make_hap";
    }

    bool init() final
    {
        if (!AbstractProcessStep::init())
            return false;

        BuildConfiguration *const bc = buildConfiguration();
        QTC_ASSERT(bc, return false);

        const FilePath cmake = CMakeProjectManager::CMakeKitAspect::cmakeExecutable(kit());
        if (cmake.isEmpty()) {
            emit addOutput(Tr::tr("No CMake tool is set in the kit."), OutputFormat::ErrorMessage);
            return false;
        }

        const QString target = makeHapTarget();
        if (target.isEmpty()) {
            emit addOutput(Tr::tr("No active build target."), OutputFormat::ErrorMessage);
            return false;
        }

        const FilePath buildDir = bc->buildDirectory();
        processParameters()->setCommandLine(
            {cmake, {"--build", buildDir.path(), "--target", target}});
        processParameters()->setWorkingDirectory(buildDir);

        // Generate the project and stop there: the two steps that follow do the
        // packaging and signing harmonydeployqt would delegate to hvigor.
        Environment env = bc->environment();
        env.unset(Constants::HVIGOR_ENV_VAR);
        // Signing inputs would make harmonydeployqt insist on a complete set for hvigor.
        for (const char *var : {Constants::SIGNING_CERT_PATH_ENV_VAR,
                                Constants::SIGNING_PROFILE_ENV_VAR,
                                Constants::SIGNING_STORE_FILE_ENV_VAR,
                                Constants::SIGNING_KEY_ALIAS_ENV_VAR,
                                Constants::SIGNING_KEY_PASSWORD_ENV_VAR,
                                Constants::SIGNING_STORE_PASSWORD_ENV_VAR,
                                Constants::SIGNING_ALG_ENV_VAR}) {
            env.unset(QLatin1String(var));
        }
        processParameters()->setEnvironment(env);
        return true;
    }

    void setupOutputFormatter(OutputFormatter *formatter) final
    {
        formatter->addLineParsers(kit()->createOutputParsers());
        AbstractProcessStep::setupOutputFormatter(formatter);
    }

};

class MakeHapStepFactory final : public BuildStepFactory
{
public:
    MakeHapStepFactory()
    {
        registerStep<MakeHapStep>(Constants::HARMONYOS_MAKE_HAP_STEP_ID);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
        setSupportedDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
        setRepeatable(false);
        setDisplayName(Tr::tr("Build HarmonyOS package (.hap)"));
    }
};


class PackageHapStep final : public AbstractProcessStep
{
public:
    PackageHapStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        setDisplayName(Tr::tr("Package HarmonyOS application with hvigor"));
        setSummaryUpdater([] { return Tr::tr("<b>Package application with hvigor</b>"); });

    }

private:
    bool init() final
    {
        if (!AbstractProcessStep::init())
            return false;

        BuildConfiguration *const bc = buildConfiguration();
        QTC_ASSERT(bc, return false);

        const FilePath hvigor = Sdk::hvigorCommand(settings().sdkLocation());
        if (hvigor.isEmpty()) {
            emit addOutput(Tr::tr("hvigor was not found in the HarmonyOS SDK."),
                           OutputFormat::ErrorMessage);
            return false;
        }

        m_buildKey = bc->activeBuildKey();
        m_project = projectDir(bc);
        m_package = packageDir(bc).pathAppended(m_buildKey + "-unsigned.hap");

        processParameters()->setCommandLine({hvigor, {"assembleHap", "--no-daemon"}});
        processParameters()->setWorkingDirectory(m_project);
        return true;
    }

    // The plugin that starts the server is built here rather than shipped: it has to match
    // the Qt it is loaded into, and everything needed for that comes from the kit.
    bool buildDebugPlugin(const FilePath &generic)
    {
        BuildConfiguration *const bc = buildConfiguration();
        QtSupport::QtVersion *const qt = QtSupport::QtKitAspect::qtVersion(bc->kit());
        const FilePath sdk = settings().sdkLocation();
        const FilePath compiler = Sdk::clangCompiler(sdk, true);
        const FilePath sysroot = Sdk::sysrootPath(sdk);
        const FilePath source = Core::ICore::resourcePath("harmonyos/qtcdebugplugin.cpp");
        if (!qt || compiler.isEmpty() || sysroot.isEmpty() || !source.exists()) {
            emit addOutput(Tr::tr("Cannot build the debug plugin; the package will not be "
                                  "debuggable."), OutputFormat::Stdout);
            return true;
        }

        const FilePath work = bc->buildDirectory().pathAppended("harmonyos-debug-plugin");
        if (const Result<> created = work.ensureWritableDir(); !created) {
            emit addOutput(created.error(), OutputFormat::ErrorMessage);
            return false;
        }
        const FilePath plugin = work.pathAppended("libqtcdebug.so");
        if (plugin.exists() && plugin.lastModified() > source.lastModified())
            return copyDebugPlugin(plugin, generic);

        const QStringList includes = {"-I" + source.parentDir().path(),
                                      "-I" + qt->headerPath().path(),
                                      "-I" + qt->headerPath().pathAppended("QtCore").path(),
                                      "-I" + qt->headerPath().pathAppended("QtGui").path()};

        // moc resolves the plugin interface id from the Qt headers, so it needs them too.
        const FilePath mocOutput = work.pathAppended("qtcdebugplugin.moc");
        Process moc;
        moc.setCommand({qt->hostLibexecPath().pathAppended("moc"),
                        QStringList(includes) << source.path() << "-o" << mocOutput.path()});
        moc.runBlocking();
        if (!mocOutput.exists()) {
            emit addOutput(Tr::tr("Running moc for the debug plugin failed: %1")
                               .arg(moc.allOutput()), OutputFormat::ErrorMessage);
            return false;
        }

        Process compile;
        compile.setCommand({compiler, QStringList{"--target=aarch64-linux-ohos",
                                                  "--sysroot=" + sysroot.path(),
                                                  "-fPIC", "-shared", "-std=c++17"}
                                          << includes << "-I" + work.path()
                                          << "-L" + qt->libraryPath().path()
                                          << "-lQt6Core" << "-lQt6Gui"
                                          << source.path() << "-o" << plugin.path()});
        compile.runBlocking();
        if (!plugin.exists()) {
            emit addOutput(Tr::tr("Building the debug plugin failed: %1")
                               .arg(compile.allOutput()), OutputFormat::ErrorMessage);
            return false;
        }
        return copyDebugPlugin(plugin, generic);
    }

    bool copyDebugPlugin(const FilePath &plugin, const FilePath &generic)
    {
        if (const Result<> created = generic.ensureWritableDir(); !created) {
            emit addOutput(created.error(), OutputFormat::ErrorMessage);
            return false;
        }
        const FilePath target = generic.pathAppended(plugin.fileName());
        target.removeFile();
        if (const Result<> copied = plugin.copyFile(target); !copied) {
            emit addOutput(copied.error(), OutputFormat::ErrorMessage);
            return false;
        }
        return true;
    }

    // The kit links a debug build against this, so it has to be beside the application's
    // own library for the loader to find it. A build whose kit did not carry the flag has
    // no dependency on it, and the copy then does no harm.
    bool shipWaitLibrary(const FilePath &libraries)
    {
        const FilePath library = Sdk::waitLibrary(settings().sdkLocation());
        if (library.isEmpty()) {
            emit addOutput(Tr::tr("Cannot build the library that holds an application at "
                                  "startup; debugging will only be able to attach to an "
                                  "application that already runs."), OutputFormat::Stdout);
            return true;
        }
        if (const Result<> created = libraries.ensureWritableDir(); !created) {
            emit addOutput(created.error(), OutputFormat::ErrorMessage);
            return false;
        }
        const FilePath target = libraries.pathAppended(library.fileName());
        target.removeFile();
        if (const Result<> copied = library.copyFile(target); !copied) {
            emit addOutput(copied.error(), OutputFormat::ErrorMessage);
            return false;
        }
        return true;
    }

    // Qt's project templates are copied from its source tree, which in an in-source build
    // holds CMake's own files as well. hvigor takes exception to them: with them present it
    // deletes the generated project mid-run and then reports the files it deleted as missing.
    void dropCMakeLeftovers()
    {
        const QStringList leftovers = {"CMakeFiles", "cmake_install.cmake", "CTestTestfile.cmake"};
        for (const QString &name : leftovers) {
            const FilePath path = m_project.pathAppended(name);
            if (!path.exists())
                continue;
            const Result<> removed = path.isDir() ? path.removeRecursively() : path.removeFile();
            if (!removed)
                emit addOutput(removed.error(), OutputFormat::Stdout);
        }
    }

    // Nothing can be launched under a debugger on the device, so a debugged application
    // starts the server itself, and both it and the plugin that starts it travel in the
    // package. hvigor packages no native package, so the server is put in afterwards,
    // but it has to be declared here, before the manifest is packaged.
    bool shipDebugPlugin()
    {
        if (!buildDebugPlugin(m_project.pathAppended("entry/libs/arm64-v8a/generic")))
            return false;
        if (!shipWaitLibrary(m_project.pathAppended("entry/libs/arm64-v8a")))
            return false;

        const FilePath moduleJson = m_project.pathAppended("entry/src/main/module.json5");
        // The server listens on a socket for the debugger on the other side of the forward.
        if (const Result<> added = addPermission(moduleJson, "ohos.permission.INTERNET"); !added) {
            emit addOutput(added.error(), OutputFormat::ErrorMessage);
            return false;
        }

        m_debugServer = packDebugServer();
        if (m_debugServer.isEmpty())
            return true;

        const Result<> declared = declareHnpPackage(moduleJson, m_debugServer.fileName());
        if (!declared) {
            emit addOutput(declared.error(), OutputFormat::ErrorMessage);
            return false;
        }
        return true;
    }

    // Returns the packed server, or nothing when the SDK does not have what it takes.
    FilePath packDebugServer()
    {
        const FilePath sdk = settings().sdkLocation();
        const FilePath server = Sdk::lldbServerForDevice(sdk);
        const FilePath hnpcli = Sdk::hnpcliCommand(sdk);
        if (server.isEmpty() || hnpcli.isEmpty()) {
            emit addOutput(Tr::tr("No debug server in the HarmonyOS SDK; the package will "
                                  "not be debuggable."), OutputFormat::Stdout);
            return {};
        }

        // What is packed and what comes out are kept apart, so that only the package itself
        // ends up in the application.
        const FilePath source = stagingDir().pathAppended("server");
        const FilePath target = stagingDir().pathAppended("package").pathAppended(hnpDirectory());
        for (const FilePath &dir : {source.pathAppended("bin"), target}) {
            if (const Result<> created = dir.ensureWritableDir(); !created) {
                emit addOutput(created.error(), OutputFormat::ErrorMessage);
                return {};
            }
        }
        const FilePath staged = source.pathAppended("bin").pathAppended(server.fileName());
        staged.removeFile();
        if (const Result<> copied = server.copyFile(staged); !copied) {
            emit addOutput(copied.error(), OutputFormat::ErrorMessage);
            return {};
        }

        Process pack;
        pack.setCommand({hnpcli, {"pack", "-i", source.nativePath(), "-o", target.nativePath(),
                                  "-n", Constants::HARMONYOS_DEBUG_SERVER_PACKAGE, "-v", "1.0"}});
        pack.runBlocking();
        const FilePath packed
            = target.pathAppended(QString(Constants::HARMONYOS_DEBUG_SERVER_PACKAGE) + ".hnp");
        if (!packed.exists()) {
            emit addOutput(Tr::tr("Packing the debug server failed: %1").arg(pack.allOutput()),
                           OutputFormat::ErrorMessage);
            return {};
        }
        return packed;
    }

    // Puts the packed server into the package hvigor built, which leaves it out. Signing
    // refuses a native package the manifest does not describe, and the device refuses to
    // install one it cannot find, so this belongs with the declaration.
    bool addDebugServerToPackage()
    {
        if (m_debugServer.isEmpty())
            return true;

        BuildConfiguration * const bc = buildConfiguration();
        QTC_ASSERT(bc, return false);
        const FilePath jar = bc->environment().searchInPath("jar");
        if (jar.isEmpty()) {
            emit addOutput(Tr::tr("No \"jar\" to put the debug server into the package with; "
                                  "the package will not be debuggable."), OutputFormat::Stdout);
            return true;
        }

        Process add;
        add.setCommand({jar, {"u0f", m_package.nativePath(), hnpDirectory().section('/', 0, 0)}});
        add.setWorkingDirectory(stagingDir().pathAppended("package"));
        add.runBlocking();
        if (add.exitCode() != 0) {
            emit addOutput(Tr::tr("Putting the debug server into the package failed: %1")
                               .arg(add.allOutput()), OutputFormat::ErrorMessage);
            return false;
        }
        return true;
    }

    FilePath stagingDir() const
    {
        return m_project.parentDir().pathAppended("harmonyos-hnp");
    }

    static QString hnpDirectory() { return "hnp/arm64-v8a"; }

    // hvigor reuses the libraries of a previous run, which then lacks whatever was staged
    // since, so its output goes before packaging.
    void dropStaleModuleOutput()
    {
        const FilePath output = m_project.pathAppended("entry/build");
        if (!output.exists())
            return;
        if (const Result<> removed = output.removeRecursively(); !removed)
            emit addOutput(removed.error(), OutputFormat::Stdout);
    }

    // The next generation replaces the project wholesale.
    bool keepPackage()
    {
        const FilePath built = m_project.pathAppended(
            "entry/build/default/outputs/default/entry-default-unsigned.hap");
        if (!built.exists()) {
            emit addOutput(Tr::tr("The package \"%1\" was not built.").arg(built.toUserOutput()),
                           OutputFormat::ErrorMessage);
            return false;
        }
        m_package.parentDir().ensureWritableDir();
        m_package.removeFile();
        if (const Result<> copied = built.copyFile(m_package); !copied) {
            emit addOutput(copied.error(), OutputFormat::ErrorMessage);
            return false;
        }
        return addDebugServerToPackage();
    }

    QtTaskTree::GroupItem runRecipe() final
    {
        using namespace QtTaskTree;

        const auto onSetup = [this](Process &process) {
            dropCMakeLeftovers();
            if (buildConfiguration()->buildType() == BuildConfiguration::Debug
                && !shipDebugPlugin()) {
                return SetupResult::StopWithError;
            }
            dropStaleModuleOutput();
            const ProvisioningProfile profile
                = readProvisioningProfile(settings().signingProfile());

            // A package can only be installed under the bundle name its profile is for.
            if (!profile.bundleName.isEmpty()) {
                const FilePath appJson = m_project.pathAppended("AppScope/app.json5");
                const Result<QString> replaced = setBundleName(appJson, profile.bundleName);
                if (!replaced) {
                    emit addOutput(replaced.error(), OutputFormat::ErrorMessage);
                    return SetupResult::StopWithError;
                }
                if (*replaced != profile.bundleName) {
                    emit addOutput(Tr::tr("Packaging as \"%1\", which is the bundle name the "
                                          "provisioning profile is for.").arg(profile.bundleName),
                                   OutputFormat::Stdout);
                }
            }

            const QString sdkVersion = Sdk::sdkVersion(settings().sdkLocation());
            if (!sdkVersion.isEmpty()) {
                const Result<QString> previous
                    = setSdkVersion(m_project.pathAppended("build-profile.json5"), sdkVersion);
                if (!previous) {
                    emit addOutput(previous.error(), OutputFormat::ErrorMessage);
                    return SetupResult::StopWithError;
                }
                if (*previous != sdkVersion) {
                    emit addOutput(Tr::tr("Packaging against SDK %1, which is the one that is "
                                          "installed.").arg(sdkVersion), OutputFormat::Stdout);
                }
            }

            QStringList unwanted;
            for (const QString &name : aclPermissions(settings().sdkLocation())) {
                if (!profile.allowedAcls.contains(name))
                    unwanted.append(name);
            }
            if (!unwanted.isEmpty()) {
                const FilePath moduleJson = m_project.pathAppended(
                    "entry/src/main/module.json5");
                const Result<QStringList> dropped = dropPermissions(moduleJson, unwanted);
                if (!dropped) {
                    emit addOutput(dropped.error(), OutputFormat::ErrorMessage);
                    return SetupResult::StopWithError;
                }
                if (!dropped->isEmpty()) {
                    // The names are the answer to "what has to be in the profile", which is
                    // worth more than a line in the output the deploy scrolls past.
                    DeploymentTask task(
                        Task::Warning,
                        Tr::tr("Dropped %1, which the provisioning profile does not allow.")
                            .arg(dropped->join(", ")));
                    task.setDetails({Tr::tr("The device grants a restricted permission only to an "
                                            "application whose profile lists it under "
                                            "\"allowed-acls\".")});
                    emit addTask(task);
                }
            }

            if (!setupProcess(process))
                return SetupResult::StopWithError;
            return SetupResult::Continue;
        };
        const auto onDone = [this](const Process &process) {
            if (!handleProcessDone(process))
                return false;
            return keepPackage();
        };
        return ProcessTask(onSetup, onDone);
    }

    void setupOutputFormatter(OutputFormatter *formatter) final
    {
        formatter->addLineParsers(kit()->createOutputParsers());
        AbstractProcessStep::setupOutputFormatter(formatter);
    }

    QString m_buildKey;
    FilePath m_project;
    FilePath m_package;
    FilePath m_debugServer;
};

class PackageHapStepFactory final : public BuildStepFactory
{
public:
    PackageHapStepFactory()
    {
        registerStep<PackageHapStep>(Constants::HARMONYOS_PACKAGE_HAP_STEP_ID);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
        setSupportedDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
        setRepeatable(false);
        setDisplayName(Tr::tr("Package HarmonyOS application with hvigor"));
    }
};

// Unlike hvigor, hap-sign-tool takes the signing material as it is issued.
class SignHapStep final : public AbstractProcessStep
{
public:
    SignHapStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        setDisplayName(Tr::tr("Sign HarmonyOS package"));
        setSummaryUpdater([] { return Tr::tr("<b>Sign package with hap-sign-tool</b>"); });
    }

private:
    bool init() final
    {
        if (!AbstractProcessStep::init())
            return false;

        BuildConfiguration *const bc = buildConfiguration();
        QTC_ASSERT(bc, return false);

        const SigningConfig config = signingConfigFromSettings(bc->environment());
        if (config.isEmpty()) {
            emit addOutput(Tr::tr("No HarmonyOS signing material is configured. The package "
                                  "cannot be installed on a device. Set it up in "
                                  "Preferences > SDKs > HarmonyOS."),
                           OutputFormat::ErrorMessage);
            return false;
        }
        const QString keyPassword = config.value(Constants::SIGNING_KEY_PASSWORD_ENV_VAR);
        const QString storePassword = config.value(Constants::SIGNING_STORE_PASSWORD_ENV_VAR);
        if (keyPassword.isEmpty() || storePassword.isEmpty()) {
            emit addOutput(Tr::tr("The HarmonyOS signing passwords are not available. Enter them "
                                  "in Preferences > SDKs > HarmonyOS, or pass them in the "
                                  "environment as %1 and %2.")
                               .arg(QLatin1String(Constants::SIGNING_KEY_PASSWORD_ENV_VAR),
                                    QLatin1String(Constants::SIGNING_STORE_PASSWORD_ENV_VAR)),
                           OutputFormat::ErrorMessage);
            return false;
        }

        const FilePath jar = Sdk::hapSignToolJar(settings().sdkLocation());
        if (jar.isEmpty()) {
            emit addOutput(Tr::tr("hap-sign-tool.jar was not found in the HarmonyOS SDK."),
                           OutputFormat::ErrorMessage);
            return false;
        }
        const FilePath java = bc->environment().searchInPath("java");
        if (java.isEmpty()) {
            emit addOutput(Tr::tr("No Java runtime was found; hap-sign-tool needs one."),
                           OutputFormat::ErrorMessage);
            return false;
        }

        const QString buildKey = bc->activeBuildKey();
        m_unsignedPackage = packageDir(bc).pathAppended(buildKey + "-unsigned.hap");
        m_signedPackage = packageDir(bc).pathAppended(buildKey + ".hap");

        CommandLine cmd{java, {"-jar", jar.nativePath(), "sign-app",
                               "-mode", "localSign",
                               "-signAlg", config.value(Constants::SIGNING_ALG_ENV_VAR,
                                                        "SHA256withECDSA"),
                               "-signCode", "1",
                               "-keyAlias", config.value(Constants::SIGNING_KEY_ALIAS_ENV_VAR),
                               "-keyPwd", keyPassword,
                               "-keystorePwd", storePassword,
                               "-keystoreFile",
                               config.value(Constants::SIGNING_STORE_FILE_ENV_VAR),
                               "-appCertFile",
                               config.value(Constants::SIGNING_CERT_PATH_ENV_VAR),
                               "-profileFile", config.value(Constants::SIGNING_PROFILE_ENV_VAR),
                               "-inFile", m_unsignedPackage.nativePath(),
                               "-outFile", m_signedPackage.nativePath()}};
        processParameters()->setCommandLine(cmd);
        processParameters()->setWorkingDirectory(packageDir(bc));
        return true;
    }

    QtTaskTree::GroupItem runRecipe() final
    {
        using namespace QtTaskTree;

        const auto onSetup = [this](Process &process) {
            // Checked here rather than in init(), which runs before the step that packages.
            if (!m_unsignedPackage.exists()) {
                emit addOutput(Tr::tr("The package \"%1\" was not built.")
                                   .arg(m_unsignedPackage.toUserOutput()),
                               OutputFormat::ErrorMessage);
                return SetupResult::StopWithError;
            }
            m_signedPackage.removeFile();
            return setupProcess(process) ? SetupResult::Continue : SetupResult::StopWithError;
        };
        const auto onDone = [this](const Process &process) {
            // hap-sign-tool reports its failures on stdout and exits successfully.
            if (!m_signedPackage.exists()) {
                emit addOutput(Tr::tr("Signing the package failed."), OutputFormat::ErrorMessage);
                return false;
            }
            return handleProcessDone(process);
        };
        return ProcessTask(onSetup, onDone);
    }

    void setupOutputFormatter(OutputFormatter *formatter) final
    {
        formatter->addLineParsers(kit()->createOutputParsers());
        AbstractProcessStep::setupOutputFormatter(formatter);
    }

    FilePath m_unsignedPackage;
    FilePath m_signedPackage;
};

class SignHapStepFactory final : public BuildStepFactory
{
public:
    SignHapStepFactory()
    {
        registerStep<SignHapStep>(Constants::HARMONYOS_SIGN_HAP_STEP_ID);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
        setSupportedDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
        setRepeatable(false);
        setDisplayName(Tr::tr("Sign HarmonyOS package"));
    }
};

// What the deploy has to know to leave the device alone: the package it installed
// there last time, and whether that is still what the device has. The device
// identifies a package by its bundle name only - the hash fields of "bm dump" are
// empty here - so the note is kept beside the package, and the device's own update
// time is what catches a package that arrived from somewhere else.

static QString forceInstallLabel()
{
    return Tr::tr("Install even when the device already has this package");
}

static FilePath installNote(const FilePath &hap)
{
    return hap.stringAppended(".installed");
}

static bool addTreeToHash(QCryptographicHash &hash, const FilePath &root)
{
    const FilePaths files = root.dirEntries(
        FileFilter({}, DirFilterFlag::Files | DirFilterFlag::Hidden,
                   DirIteratorFlag::Subdirectories));
    if (files.isEmpty())
        return false;

    QStringList relative;
    QHash<QString, FilePath> byName;
    for (const FilePath &file : files) {
        const QString name = file.relativePathFromDir(root);
        // hvigor builds into the first and caches into the second; the package holds
        // neither, and both change on every run.
        if (name.startsWith("build/") || name.startsWith(".cxx/"))
            continue;
        relative.append(name);
        byName.insert(name, file);
    }
    relative.sort();

    for (const QString &name : relative) {
        const Result<QByteArray> contents = byName.value(name).fileContents();
        if (!contents)
            return false;
        hash.addData(name.toUtf8());
        hash.addData(*contents);
    }
    return true;
}

// Not the .hap itself: repackaging the very same content yields different bytes every
// time, so the answer has to come from what goes in. Measured, the generated project is
// byte-stable across runs apart from hvigor's own scratch directories, which nothing is
// packaged from. The debug server is added to the package from a directory beside the
// generated project, so its staged content counts too - without it a package holding a
// different lldb-server looks unchanged, and the install is skipped.
static QString packagedContentFingerprint(const FilePath &project)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!addTreeToHash(hash, project.pathAppended("entry")))
        return {};
    const FilePath staged = project.parentDir().pathAppended("harmonyos-hnp/server");
    if (staged.isDir() && !addTreeToHash(hash, staged))
        return {};
    return QString::fromLatin1(hash.result().toHex());
}

// The note is three fields; all of them have to still hold for the package on the
// device to be the one this step put there.
static bool noteHolds(const QByteArray &note, const QString &serial, const QString &fingerprint,
                      const QString &updated)
{
    if (fingerprint.isEmpty() || updated.isEmpty())
        return false;

    QHash<QString, QString> was;
    for (const QString &line : QString::fromUtf8(note).split('\n', Qt::SkipEmptyParts)) {
        const qsizetype at = line.indexOf('=');
        if (at > 0)
            was.insert(line.left(at), line.mid(at + 1));
    }
    return was.value("serial") == serial && was.value("package") == fingerprint
           && was.value("updated") == updated;
}

// Empty when the device does not have the bundle at all, which is an answer too.
static QString installedUpdateTime(const QString &serial, const QString &bundle)
{
    const FilePath hdc = Sdk::hdcCommand(settings().sdkLocation());
    if (hdc.isEmpty() || bundle.isEmpty())
        return {};

    CommandLine cmd{hdc};
    if (!serial.isEmpty())
        cmd.addArgs({"-t", serial});
    cmd.addArgs({"shell", "bm", "dump", "-n", bundle});

    Process process;
    process.setCommand(cmd);
    process.setEnvironment(Sdk::hdcEnvironment());
    process.runBlocking(std::chrono::seconds(5));

    static const QRegularExpression re("\"updateTime\"\\s*:\\s*(\\d+)");
    const QRegularExpressionMatch match = re.match(process.cleanedStdOut());
    return match.hasMatch() ? match.captured(1) : QString();
}

// Installs the built .hap onto the connected device with hdc.
class InstallHapStep final : public AbstractProcessStep
{
public:
    InstallHapStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        setDisplayName(Tr::tr("Install HarmonyOS package with hdc"));
        setSummaryUpdater([] { return Tr::tr("<b>Install .hap on device with hdc</b>"); });

        m_force.setSettingsKey("HarmonyOs.InstallHapStep.AlwaysInstall");
        m_force.setLabel(forceInstallLabel(), BoolAspect::LabelPlacement::AtCheckBox);
        m_force.setValue(false);
    }

private:
    bool init() final
    {
        if (!AbstractProcessStep::init())
            return false;

        BuildConfiguration *const bc = buildConfiguration();
        QTC_ASSERT(bc, return false);

        const FilePath hdc = Sdk::hdcCommand(settings().sdkLocation());
        if (hdc.isEmpty()) {
            emit addOutput(Tr::tr("hdc was not found in the HarmonyOS SDK."),
                           OutputFormat::ErrorMessage);
            return false;
        }

        const QString buildKey = bc->activeBuildKey();
        if (buildKey.isEmpty()) {
            emit addOutput(Tr::tr("No active build target."), OutputFormat::ErrorMessage);
            return false;
        }

        m_hap = packageDir(bc).pathAppended(buildKey + ".hap");
        m_project = projectDir(bc);
        m_bundle = bundleName(bc->buildDirectory());
        m_serial.clear();
        if (const auto device = std::dynamic_pointer_cast<const HarmonyOsDevice>(
                RunDeviceKitAspect::device(kit()))) {
            m_serial = device->serialNumber();
        }

        // Target the run device explicitly so the right one is used when several
        // are connected.
        CommandLine cmd{hdc};
        if (!m_serial.isEmpty())
            cmd.addArgs({"-t", m_serial});
        cmd.addArgs({"install", m_hap.nativePath()});
        processParameters()->setCommandLine(cmd);
        processParameters()->setEnvironment(Sdk::hdcEnvironment());
        processParameters()->setWorkingDirectory(bc->buildDirectory());
        return true;
    }

    void setupOutputFormatter(OutputFormatter *formatter) final
    {
        formatter->addLineParsers(kit()->createOutputParsers());
        AbstractProcessStep::setupOutputFormatter(formatter);
    }

    QtTaskTree::GroupItem runRecipe() final
    {
        using namespace QtTaskTree;

        const auto onSetup = [this](Process &process) {
            // Checked here rather than in init(), which runs before the steps that
            // build the package.
            if (!m_hap.exists()) {
                emit addOutput(Tr::tr("The package \"%1\" was not built.")
                                   .arg(m_hap.toUserOutput()),
                               OutputFormat::ErrorMessage);
                return SetupResult::StopWithError;
            }
            m_fingerprint = packagedContentFingerprint(m_project);
            if (!m_force() && deviceHasThisPackage()) {
                emit addOutput(Tr::tr("The device already has this package, so it was not "
                                      "installed again. \"%1\" installs it regardless.")
                                   .arg(forceInstallLabel()),
                               OutputFormat::NormalMessage);
                return SetupResult::StopWithSuccess;
            }
            if (!setupProcess(process))
                return SetupResult::StopWithError;
            m_output.clear();
            process.setStdOutCallback([this](const QString &text) {
                m_output += text;
                emit addOutput(text, OutputFormat::Stdout, DontAppendNewline);
            });
            return SetupResult::Continue;
        };
        const auto onDone = [this](const Process &process) {
            // hdc exits successfully even when it did not install anything, so the
            // outcome has to be read off its output. It reports a package it could
            // not read with "[Fail]" and one the device rejected with "msg:error:".
            if (m_output.contains("[Fail]") || m_output.contains("msg:error:")) {
                emit addOutput(Tr::tr("Installing the package on the device failed."),
                               OutputFormat::ErrorMessage);
                return false;
            }
            if (!handleProcessDone(process))
                return false;
            noteTheInstall();
            return true;
        };
        return ProcessTask(onSetup, onDone);
    }

    // Only what this step installed, on this device, and still untouched there. The
    // device is asked last, being the only part that costs a round trip.
    bool deviceHasThisPackage()
    {
        const Result<QByteArray> noted = installNote(m_hap).fileContents();
        if (!noted || m_fingerprint.isEmpty())
            return false;
        return noteHolds(*noted, m_serial, m_fingerprint,
                         installedUpdateTime(m_serial, m_bundle));
    }

    void noteTheInstall()
    {
        const QString updated = installedUpdateTime(m_serial, m_bundle);
        if (m_fingerprint.isEmpty() || updated.isEmpty()) {
            // Nothing to compare against next time, so let that run install again.
            installNote(m_hap).removeFile();
            return;
        }
        const QString note = "serial=" + m_serial + "\npackage=" + m_fingerprint
                             + "\nupdated=" + updated + "\n";
        if (const Result<qint64> written = installNote(m_hap).writeFileContents(note.toUtf8());
            !written) {
            emit addOutput(written.error(), OutputFormat::Stdout);
        }
    }

    FilePath m_hap;
    FilePath m_project;
    QString m_output;
    QString m_serial;
    QString m_bundle;
    QString m_fingerprint;
    BoolAspect m_force{this};
};

class InstallHapStepFactory final : public BuildStepFactory
{
public:
    InstallHapStepFactory()
    {
        registerStep<InstallHapStep>(Constants::HARMONYOS_INSTALL_HAP_STEP_ID);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
        setSupportedDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
        setRepeatable(false);
        setDisplayName(Tr::tr("Install HarmonyOS package with hdc"));
    }
};

class HarmonyOsDeployConfigurationFactory final : public DeployConfigurationFactory
{
public:
    HarmonyOsDeployConfigurationFactory()
    {
        setConfigBaseId(Constants::HARMONYOS_DEPLOY_CONFIG_ID);
        addSupportedTargetDeviceType(Constants::HARMONYOS_DEVICE_TYPE);
        setDefaultDisplayName(Tr::tr("Deploy to HarmonyOS device"));
        addInitialStep(Constants::HARMONYOS_MAKE_HAP_STEP_ID);
        addInitialStep(Constants::HARMONYOS_PACKAGE_HAP_STEP_ID);
        addInitialStep(Constants::HARMONYOS_SIGN_HAP_STEP_ID);
        addInitialStep(Constants::HARMONYOS_INSTALL_HAP_STEP_ID);
    }
};

void setupHarmonyOsDeployStep()
{
    static MakeHapStepFactory theMakeHapStepFactory;
    static PackageHapStepFactory thePackageHapStepFactory;
    static SignHapStepFactory theSignHapStepFactory;
    static InstallHapStepFactory theInstallHapStepFactory;
}

void setupHarmonyOsDeployConfiguration()
{
    static HarmonyOsDeployConfigurationFactory theHarmonyOsDeployConfigurationFactory;
}

#ifdef WITH_TESTS

class HarmonyOsManifestTest final : public QObject
{
    Q_OBJECT

private slots:
    void testDeclareHnpPackage()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath moduleJson = FilePath::fromString(dir.filePath("module.json5"));
        QVERIFY(moduleJson.writeFileContents(manifest()));

        QVERIFY(declareHnpPackage(moduleJson, "lldbserver.hnp"));
        const QString once = text(moduleJson);
        QVERIFY(once.contains("\"hnpPackages\""));
        QVERIFY(once.contains("\"package\": \"lldbserver.hnp\""));

        QVERIFY(declareHnpPackage(moduleJson, "lldbserver.hnp"));
        QCOMPARE(text(moduleJson).count("hnpPackages"), 1);
    }

    void testAddPermission()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath moduleJson = FilePath::fromString(dir.filePath("module.json5"));
        QVERIFY(moduleJson.writeFileContents(manifest()));

        QVERIFY(addPermission(moduleJson, "ohos.permission.INTERNET"));
        QCOMPARE(text(moduleJson).count("ohos.permission.INTERNET"), 1);
        QVERIFY(text(moduleJson).contains("ohos.permission.STORE_PERSISTENT_DATA"));

        QVERIFY(addPermission(moduleJson, "ohos.permission.INTERNET"));
        QCOMPARE(text(moduleJson).count("ohos.permission.INTERNET"), 1);
    }

    void testReadProvisioningProfile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath profile = FilePath::fromString(dir.filePath("profile.p7b"));
        // As in a real profile: DER bytes with a stray brace ahead of the JSON.
        const QByteArray blob = QByteArray("0\x82\x0f{\x06\x09*\x86H")
            + R"({"version-name":"2.0.0","bundle-info":{"bundle-name":"Qt.Greatest.App"},)"
              R"("acls":{"allowed-acls":["ohos.permission.READ_PASTEBOARD"]}})";
        QVERIFY(profile.writeFileContents(blob));

        const ProvisioningProfile read = readProvisioningProfile(profile);
        QCOMPARE(read.bundleName, QString("Qt.Greatest.App"));
        QCOMPARE(read.allowedAcls, QStringList{"ohos.permission.READ_PASTEBOARD"});
    }

    void testDebugPluginSourceIsShipped()
    {
        const FilePath source = Core::ICore::resourcePath("harmonyos/qtcdebugplugin.cpp");
        QVERIFY2(source.exists(), qPrintable(source.toUserOutput()));
        const Result<QByteArray> contents = source.fileContents();
        QVERIFY(contents);
        QVERIFY(contents->contains("QGenericPluginFactoryInterface_iid"));
        QVERIFY(contents->contains("lldb-server"));
        QVERIFY(Core::ICore::resourcePath("harmonyos/qtcdebug.json").exists());
    }

    // What comes back is what the deploy step reports as the permissions to have the
    // profile allow, so a name that goes missing here takes the diagnostic with it.
    void testDropPermissions()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath moduleJson = FilePath::fromString(dir.filePath("module.json5"));
        QVERIFY(moduleJson.writeFileContents(manifest()));

        const Result<QStringList> dropped = dropPermissions(
            moduleJson, {"ohos.permission.STORE_PERSISTENT_DATA", "ohos.permission.CAMERA"});
        QVERIFY(dropped);
        QCOMPARE(*dropped, QStringList{"ohos.permission.STORE_PERSISTENT_DATA"});
        QVERIFY(!text(moduleJson).contains("ohos.permission.STORE_PERSISTENT_DATA"));
        // The entry beside it, and the manifest around it, have to survive.
        QVERIFY(text(moduleJson).contains("ohos.permission.FILE_ACCESS_PERSIST"));
        QVERIFY(text(moduleJson).contains("\"mainElement\": \"QAbility\""));
    }

    void testInstallNote()
    {
        const QByteArray note = "serial=1.2.3.4:5\npackage=abc123\nupdated=1788191262345\n";
        QVERIFY(noteHolds(note, "1.2.3.4:5", "abc123", "1788191262345"));
        // Another device, another build, and a package that was replaced behind our back.
        QVERIFY(!noteHolds(note, "9.9.9.9:9", "abc123", "1788191262345"));
        QVERIFY(!noteHolds(note, "1.2.3.4:5", "def456", "1788191262345"));
        QVERIFY(!noteHolds(note, "1.2.3.4:5", "abc123", "1788191262999"));
        // No package on the device at all, which is what an empty update time means.
        QVERIFY(!noteHolds(note, "1.2.3.4:5", "abc123", ""));
        QVERIFY(!noteHolds("", "1.2.3.4:5", "abc123", "1788191262345"));
    }

    void testFingerprintCoversStagedServer()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath root = FilePath::fromString(dir.path());
        const FilePath project = root.pathAppended("harmonyos-project");
        QVERIFY(project.pathAppended("entry").ensureWritableDir());
        QVERIFY(project.pathAppended("entry/module.json5").writeFileContents("{}"));
        const QString bare = packagedContentFingerprint(project);
        QVERIFY(!bare.isEmpty());

        const FilePath staged = root.pathAppended("harmonyos-hnp/server/bin");
        QVERIFY(staged.ensureWritableDir());
        QVERIFY(staged.pathAppended("lldb-server").writeFileContents("one"));
        const QString withServer = packagedContentFingerprint(project);
        QVERIFY(withServer != bare);

        QVERIFY(staged.pathAppended("lldb-server").writeFileContents("another"));
        QVERIFY(packagedContentFingerprint(project) != withServer);
    }

    void testAddPermissionWithoutList()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath moduleJson = FilePath::fromString(dir.filePath("module.json5"));
        QVERIFY(moduleJson.writeFileContents(R"({ "module": { "name": "entry" } })"));

        QVERIFY(!addPermission(moduleJson, "ohos.permission.INTERNET"));
    }

private:
    static QByteArray manifest()
    {
        return R"({
  "module": {
    "name": "entry",
    "type": "entry",
    "mainElement": "QAbility",
    "requestPermissions": [
        {
            "name": "ohos.permission.STORE_PERSISTENT_DATA"
        },
        {
            "name": "ohos.permission.FILE_ACCESS_PERSIST"
        }
    ]
  }
})";
    }

    static QString text(const FilePath &path)
    {
        const Result<QByteArray> contents = path.fileContents();
        return contents ? QString::fromUtf8(*contents) : QString();
    }
};

QObject *createHarmonyOsManifestTest()
{
    return new HarmonyOsManifestTest;
}

#endif // WITH_TESTS

} // namespace HarmonyOs::Internal

#ifdef WITH_TESTS
#include "harmonyosdeploystep.moc"
#endif
