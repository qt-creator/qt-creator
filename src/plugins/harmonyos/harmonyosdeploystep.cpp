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
#include <utils/environment.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/store.h>
#include <utils/stringutils.h>

#include <QJsonDocument>
#include <QJsonObject>

using namespace ProjectExplorer;
using namespace Utils;

namespace HarmonyOs::Internal {

// Signing values keyed by the QT_HARMONYOS_SIGNING_* environment variable name.
using SigningConfig = QMap<QString, QString>;

const char SIGNING_CACHE_KEY[] = "HarmonyOS.SigningCache";

// The key and store passwords are secrets. They are applied to the build but
// never written to the project .user file; they are re-read from
// build-profile.json5 each session instead.
static bool isSecretSigningVar(const QString &var)
{
    return var == Constants::SIGNING_KEY_PASSWORD_ENV_VAR
        || var == Constants::SIGNING_STORE_PASSWORD_ENV_VAR;
}

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

// Reads the signingConfigs.material block that DevEco's automatic signing writes
// into the project's harmonyos-build/build-profile.json5, mapping each field to
// its QT_HARMONYOS_SIGNING_* variable. Empty when no signing material is present.
static SigningConfig readSigningConfig(const FilePath &buildDir)
{
    SigningConfig config;
    const FilePath profile = buildDir.pathAppended("harmonyos-build/build-profile.json5");
    const Result<QByteArray> contents = profile.fileContents();
    if (!contents)
        return config;

    const QString text = QString::fromUtf8(Utils::removeCommentsFromJson(*contents));
    const qsizetype materialStart = text.indexOf("\"material\"");
    if (materialStart < 0)
        return config;
    // The material block is a flat leaf object, so it parses as plain JSON once
    // the surrounding JSON5 comments have been stripped.
    const QString object = balancedBraces(text, materialStart);
    if (object.isEmpty())
        return config;
    const QJsonObject material = QJsonDocument::fromJson(object.toUtf8()).object();

    const QPair<QString, QString> fields[] = {
        {"certpath", Constants::SIGNING_CERT_PATH_ENV_VAR},
        {"profile", Constants::SIGNING_PROFILE_ENV_VAR},
        {"storeFile", Constants::SIGNING_STORE_FILE_ENV_VAR},
        {"keyAlias", Constants::SIGNING_KEY_ALIAS_ENV_VAR},
        {"keyPassword", Constants::SIGNING_KEY_PASSWORD_ENV_VAR},
        {"storePassword", Constants::SIGNING_STORE_PASSWORD_ENV_VAR},
        {"signAlg", Constants::SIGNING_ALG_ENV_VAR},
    };
    for (const auto &[key, var] : fields) {
        const QJsonValue value = material.value(key);
        if (value.isString())
            config.insert(var, value.toString());
    }
    return config;
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

        // make_hap regenerates build-profile.json5 and drops the signing config
        // on every run, so cache the material and re-apply it even after a run
        // (or another tool) has cleared it. Secrets are not cached to disk (see
        // toMap), so the passwords are re-read here on every session.
        const SigningConfig live = readSigningConfig(buildDir);
        if (!live.isEmpty())
            m_signingCache = live;

        Environment env = bc->environment();
        if (m_signingCache.isEmpty()) {
            emit addOutput(Tr::tr("No HarmonyOS signing configuration was found. The package "
                                  "will be unsigned and cannot be installed on a device. Open "
                                  "the harmonyos-build folder in DevEco Studio to set up "
                                  "automatic signing."),
                           OutputFormat::Stdout);
        } else {
            for (auto it = m_signingCache.cbegin(); it != m_signingCache.cend(); ++it)
                env.set(it.key(), it.value());
            if (!m_signingCache.contains(Constants::SIGNING_KEY_PASSWORD_ENV_VAR)
                || !m_signingCache.contains(Constants::SIGNING_STORE_PASSWORD_ENV_VAR)) {
                emit addOutput(Tr::tr("The HarmonyOS signing passwords are not available. Open "
                                      "the harmonyos-build folder in DevEco Studio to refresh "
                                      "the signing configuration."),
                               OutputFormat::Stdout);
            }
        }
        processParameters()->setEnvironment(env);
        return true;
    }

    void toMap(Store &map) const final
    {
        AbstractProcessStep::toMap(map);
        QVariantMap cache;
        for (auto it = m_signingCache.cbegin(); it != m_signingCache.cend(); ++it) {
            if (!isSecretSigningVar(it.key()))
                cache.insert(it.key(), it.value());
        }
        map.insert(SIGNING_CACHE_KEY, cache);
    }

    void fromMap(const Store &map) final
    {
        AbstractProcessStep::fromMap(map);
        m_signingCache.clear();
        const QVariantMap cache = map.value(SIGNING_CACHE_KEY).toMap();
        for (auto it = cache.cbegin(); it != cache.cend(); ++it)
            m_signingCache.insert(it.key(), it.value().toString());
    }

    void setupOutputFormatter(OutputFormatter *formatter) final
    {
        formatter->addLineParsers(kit()->createOutputParsers());
        AbstractProcessStep::setupOutputFormatter(formatter);
    }

    SigningConfig m_signingCache;
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

        const FilePath hap = bc->buildDirectory().pathAppended(
            "harmonyos-build/entry/build/default/outputs/default/" + buildKey + ".hap");

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
        addInitialStep(Constants::HARMONYOS_INSTALL_HAP_STEP_ID);
    }
};

void setupHarmonyOsDeployStep()
{
    static MakeHapStepFactory theMakeHapStepFactory;
    static InstallHapStepFactory theInstallHapStepFactory;
}

void setupHarmonyOsDeployConfiguration()
{
    static HarmonyOsDeployConfigurationFactory theHarmonyOsDeployConfigurationFactory;
}

} // namespace HarmonyOs::Internal
