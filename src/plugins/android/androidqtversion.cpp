// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidmanager.h"
#include "androidqtversion.h"
#include "androidtr.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/buildsystem.h>

#include <proparser/profileevaluator.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#ifdef WITH_TESTS
#   include <QTest>
#   include "androidplugin.h"
#endif // WITH_TESTS

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidQtVersion::AndroidQtVersion()
    : QtSupport::QtVersion()
    , m_guard(std::make_unique<QObject>())
{
    QObject::connect(AndroidConfigurations::instance(),
                     &AndroidConfigurations::aboutToUpdate,
                     m_guard.get(),
                     [this] { resetCache(); });
}

bool AndroidQtVersion::isValid() const
{
    if (!QtVersion::isValid())
        return false;
    if (qtAbis().isEmpty())
        return false;
    return true;
}

QString AndroidQtVersion::invalidReason() const
{
    QString tmp = QtVersion::invalidReason();
    if (tmp.isEmpty()) {
        if (AndroidConfigurations::currentConfig().ndkLocation(this).isEmpty())
            return Tr::tr("NDK is not configured in Devices > Android.");
        if (AndroidConfigurations::currentConfig().sdkLocation().isEmpty())
            return Tr::tr("SDK is not configured in Devices > Android.");
        if (qtAbis().isEmpty())
            return Tr::tr("Failed to detect the ABIs used by the Qt version. Check the settings in "
                          "Devices > Android for errors.");
    }
    return tmp;
}

bool AndroidQtVersion::supportsMultipleQtAbis() const
{
    return qtVersion() >= QVersionNumber(5, 14) && qtVersion() < QVersionNumber(6, 0);
}

Abis AndroidQtVersion::detectQtAbis() const
{
    const bool conf = AndroidConfigurations::currentConfig().sdkFullyConfigured();
    return conf ? Utils::transform<Abis>(androidAbis(), &AndroidManager::androidAbi2Abi) : Abis();
}

void AndroidQtVersion::addToEnvironment(const Kit *k, Utils::Environment &env) const
{
    QtVersion::addToEnvironment(k, env);

    const AndroidConfig &config = AndroidConfigurations::currentConfig();
    // this env vars are used by qmake mkspecs to generate makefiles (check QTDIR/mkspecs/android-g++/qmake.conf for more info)
    env.set(QLatin1String("ANDROID_NDK_HOST"), config.toolchainHost(this));
    env.set(QLatin1String("ANDROID_NDK_ROOT"), config.ndkLocation(this).toUserOutput());
    env.set(QLatin1String("ANDROID_NDK_PLATFORM"),
            config.bestNdkPlatformMatch(qMax(minimumNDK(), AndroidManager::minimumSDK(k)), this));
}

void AndroidQtVersion::setupQmakeRunEnvironment(Utils::Environment &env) const
{
    env.set(QLatin1String("ANDROID_NDK_ROOT"),
            AndroidConfigurations::currentConfig().ndkLocation(this).toUserOutput());
}

QString AndroidQtVersion::description() const
{
    //: Qt Version is meant for Android
    return Tr::tr("Android");
}

const QStringList &AndroidQtVersion::androidAbis() const
{
    if (m_androidAbis.isEmpty()) {
        bool sanityCheckNotUsed;
        const BuiltWith bw = builtWith(&sanityCheckNotUsed);
        if (!bw.androidAbi.isEmpty()) {
            m_androidAbis << bw.androidAbi;
            m_minNdk = bw.apiVersion;
        } else {
            ensureMkSpecParsed();
        }
    }

    return m_androidAbis;
}

int AndroidQtVersion::minimumNDK() const
{
    if (m_minNdk == -1)
        ensureMkSpecParsed();
    return m_minNdk;
}

QString AndroidQtVersion::androidDeploymentSettingsFileName(const Target *target)
{
    const BuildSystem *bs = target->buildSystem();
    if (!bs)
        return {};
    const QString buildKey = target->activeBuildKey();
    const QString displayName = bs->buildTarget(buildKey).displayName;
    const QString fileName = AndroidManager::isQt5CmakeProject(target)
                                 ? QLatin1String("android_deployment_settings.json")
                                 : QString::fromLatin1("android-%1-deployment-settings.json")
                                       .arg(displayName);
    return fileName;
}

Utils::FilePath AndroidQtVersion::androidDeploymentSettings(const Target *target)
{
    // Try to fetch the file name from node data as provided by qmake and Qbs
    QString buildKey = target->activeBuildKey();
    const ProjectNode *node = target->project()->findNodeForBuildKey(buildKey);
    if (node) {
        const QString nameFromData = node->data(Constants::AndroidDeploySettingsFile).toString();
        if (!nameFromData.isEmpty())
            return Utils::FilePath::fromUserInput(nameFromData);
    }

    // If unavailable, construct the name by ourselves (CMake)
    const QString fileName = androidDeploymentSettingsFileName(target);
    return AndroidManager::buildDirectory(target) / fileName;
}

AndroidQtVersion::BuiltWith AndroidQtVersion::builtWith(bool *ok) const
{
    const Utils::FilePath coreModuleJson = qmakeFilePath().parentDir().parentDir()
                                           // version.prefix() not yet set when this is called
                                           / "modules/Core.json";
    if (coreModuleJson.exists()) {
        Utils::FileReader reader;
        if (reader.fetch(coreModuleJson))
            return parseBuiltWith(reader.data(), ok);
    }

    if (ok)
        *ok = false;
    return {};
}

static int versionFromPlatformString(const QString &string, bool *ok = nullptr)
{
    static const QRegularExpression regex("android-(\\d+)");
    const QRegularExpressionMatch match = regex.match(string);
    if (ok)
        *ok = false;
    return match.hasMatch() ? match.captured(1).toInt(ok) : -1;
}

static QString abiFromCompilerTarget(const QString &string)
{
    const QStringList components = string.split("-");
    if (components.isEmpty())
        return {};

    QString qtAbi;
    const QString compilerAbi = components.first();
    if (compilerAbi == Constants::AArch64ToolsDisplayName)
        qtAbi = ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A;
    else if (compilerAbi == Constants::ArmV7ToolsDisplayName)
        qtAbi = ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A;
    else if (compilerAbi == Constants::X86_64ToolsDisplayName)
        qtAbi = ProjectExplorer::Constants::ANDROID_ABI_X86_64;
    else if (compilerAbi == Constants::X86ToolsDisplayName)
        qtAbi = ProjectExplorer::Constants::ANDROID_ABI_X86;
    return qtAbi;
}

AndroidQtVersion::BuiltWith AndroidQtVersion::parseBuiltWith(const QByteArray &modulesCoreJsonData,
                                                             bool *ok)
{
    bool validPlatformString = false;
    AndroidQtVersion::BuiltWith result;
    const QJsonObject jsonObject = QJsonDocument::fromJson(modulesCoreJsonData).object();
    if (const QJsonValue builtWith = jsonObject.value("built_with"); !builtWith.isUndefined()) {
        if (const QJsonValue android = builtWith["android"]; !android.isUndefined()) {
            if (const QJsonValue apiVersion = android["api_version"]; !apiVersion.isUndefined()) {
                const QString apiVersionString = apiVersion.toString();
                const int v = versionFromPlatformString(apiVersionString, &validPlatformString);
                if (validPlatformString)
                    result.apiVersion = v;
            }
            if (const QJsonValue ndk = android["ndk"]; !ndk.isUndefined()) {
                if (const QJsonValue version = ndk["version"]; !version.isUndefined())
                    result.ndkVersion = QVersionNumber::fromString(version.toString());
            }
        }
        if (const QJsonValue compilerTarget = builtWith["compiler_target"];
            !compilerTarget.isUndefined()) {
            result.androidAbi = abiFromCompilerTarget(compilerTarget.toString());
        }
    }

    if (ok)
        *ok = validPlatformString && !result.ndkVersion.isNull();
    return result;
}

void AndroidQtVersion::parseMkSpec(ProFileEvaluator *evaluator) const
{
    m_androidAbis = evaluator->values("ALL_ANDROID_ABIS");
    if (m_androidAbis.isEmpty())
        m_androidAbis = QStringList{evaluator->value(Constants::ANDROID_TARGET_ARCH)};
    const QString androidPlatform = evaluator->value("ANDROID_PLATFORM");
    if (!androidPlatform.isEmpty())
        m_minNdk = versionFromPlatformString(androidPlatform);
    QtVersion::parseMkSpec(evaluator);
}

QSet<Utils::Id> AndroidQtVersion::availableFeatures() const
{
    QSet<Utils::Id> features = QtSupport::QtVersion::availableFeatures();
    features.insert(QtSupport::Constants::FEATURE_MOBILE);
    features.remove(QtSupport::Constants::FEATURE_QT_CONSOLE);
    features.remove(QtSupport::Constants::FEATURE_QT_WEBKIT);
    return features;
}

QSet<Utils::Id> AndroidQtVersion::targetDeviceTypes() const
{
    return {Constants::ANDROID_DEVICE_TYPE};
}


// Factory

AndroidQtVersionFactory::AndroidQtVersionFactory()
{
    setQtVersionCreator([] { return new AndroidQtVersion; });
    setSupportedType(Constants::ANDROID_QT_TYPE);
    setPriority(90);

    setRestrictionChecker([](const SetupData &setup) {
        return !setup.config.contains("android-no-sdk")
                && (setup.config.contains("android")
                    || setup.platforms.contains("android"));
    });
}

#ifdef WITH_TESTS
void AndroidPlugin::testAndroidQtVersionParseBuiltWith_data()
{
    QTest::addColumn<QString>("modulesCoreJson");
    QTest::addColumn<bool>("hasInfo");
    QTest::addColumn<QVersionNumber>("ndkVersion");
    QTest::addColumn<int>("apiVersion");

    QTest::newRow("Android Qt 6.4")
        << R"({
                "module_name": "Core",
                "version": "6.4.1",
                "built_with": {
                    "compiler_id": "Clang",
                    "compiler_target": "x86_64-none-linux-android23",
                    "compiler_version": "12.0.8",
                    "cross_compiled": true,
                    "target_system": "Android"
                }
            })"
        << false
        << QVersionNumber()
        << -1;

    QTest::newRow("Android Qt 6.5")
        << R"({
                "module_name": "Core",
                "version": "6.5.0",
                "built_with": {
                    "android": {
                        "api_version": "android-31",
                        "ndk": {
                            "version": "25.1.8937393"
                        }
                    },
                    "compiler_id": "Clang",
                    "compiler_target": "i686-none-linux-android23",
                    "compiler_version": "14.0.6",
                    "cross_compiled": true,
                    "target_system": "Android"
                }
            })"
        << true
        << QVersionNumber(25, 1, 8937393)
        << 31;
}

void AndroidPlugin::testAndroidQtVersionParseBuiltWith()
{
    QFETCH(QString, modulesCoreJson);
    QFETCH(bool, hasInfo);
    QFETCH(int, apiVersion);
    QFETCH(QVersionNumber, ndkVersion);

    bool ok = false;
    const AndroidQtVersion::BuiltWith bw =
            AndroidQtVersion::parseBuiltWith(modulesCoreJson.toUtf8(), &ok);
    QCOMPARE(ok, hasInfo);
    QCOMPARE(bw.apiVersion, apiVersion);
    QCOMPARE(bw.ndkVersion, ndkVersion);
}
#endif // WITH_TESTS

} // Internal
} // Android
