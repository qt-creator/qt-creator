// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosdeploystep.h"

#include "harmonyosconstants.h"
#include "harmonyosdevice.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"
#include "harmonyostr.h"

#include <cmakeprojectmanager/cmakekitaspect.h>

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/commandline.h>
#include <utils/qtcprocess.h>
#include <utils/environment.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/store.h>
#include <utils/stringutils.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace Utils;

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

static ProvisioningProfile readProvisioningProfile(const FilePath &profile)
{
    ProvisioningProfile result;
    const Result<QByteArray> contents = profile.fileContents();
    if (!contents)
        return result;

    const QString text = QString::fromLatin1(*contents);
    const QString object = balancedBraces(text, 0);
    if (object.isEmpty())
        return result;
    const QJsonObject json = QJsonDocument::fromJson(object.toUtf8()).object();
    result.bundleName = json.value("bundle-info").toObject().value("bundle-name").toString();
    for (const QJsonValue &acl : json.value("acls").toObject().value("allowed-acls").toArray())
        result.allowedAcls.append(acl.toString());
    return result;
}

// The permissions Qt asks for that a device only grants when the profile allows them.
static QStringList aclPermissions()
{
    return {"ohos.permission.READ_PASTEBOARD"};
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

    // The libraries Qt needs at run time, which harmonydeployqt does not stage.
    bool addAdditionalPackages()
    {
        const FilePath libs = settings().additionalPackages();
        if (libs.isEmpty())
            return true;

        const FilePaths abiDirs = m_project.pathAppended("entry/libs").dirEntries(
            FileFilter({}, DirFilterFlag::Dirs | DirFilterFlag::NoDotAndDotDot));
        for (const FilePath &abiDir : abiDirs) {
            const FilePaths sources = libs.dirEntries(
                FileFilter({"*.so"}, DirFilterFlag::Files));
            for (const FilePath &source : sources) {
                const FilePath target = abiDir / source.fileName();
                if (target.exists())
                    continue;
                if (const Result<> copied = source.copyFile(target); !copied) {
                    emit addOutput(copied.error(), OutputFormat::ErrorMessage);
                    return false;
                }
            }
        }
        return true;
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
        return true;
    }

    QtTaskTree::GroupItem runRecipe() final
    {
        using namespace QtTaskTree;

        const auto onSetup = [this](Process &process) {
            if (!addAdditionalPackages())
                return SetupResult::StopWithError;
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

            QStringList unwanted;
            for (const QString &name : aclPermissions()) {
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
                for (const QString &name : *dropped) {
                    emit addOutput(Tr::tr("Dropped the request for %1, which the provisioning "
                                          "profile does not allow.").arg(name),
                                   OutputFormat::Stdout);
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

// Installs the built .hap onto the connected device with hdc.
class InstallHapStep final : public AbstractProcessStep
{
public:
    InstallHapStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        setDisplayName(Tr::tr("Install HarmonyOS package with hdc"));
        setSummaryUpdater([] { return Tr::tr("<b>Install .hap on device with hdc</b>"); });
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
        const FilePath &hap = m_hap;

        // Target the run device explicitly so the right one is used when several
        // are connected.
        CommandLine cmd{hdc};
        if (const auto device = std::dynamic_pointer_cast<const HarmonyOsDevice>(
                RunDeviceKitAspect::device(kit()))) {
            if (const QString serial = device->serialNumber(); !serial.isEmpty())
                cmd.addArgs({"-t", serial});
        }
        cmd.addArgs({"install", hap.nativePath()});
        processParameters()->setCommandLine(cmd);
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
            return handleProcessDone(process);
        };
        return ProcessTask(onSetup, onDone);
    }

    FilePath m_hap;
    QString m_output;
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

} // namespace HarmonyOs::Internal
