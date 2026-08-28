// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidmcpsupport.h"

#include "androidconfigurations.h"
#include "androidconstants.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionmanager.h>

#include <mcp/server/toolregistry.h>

#include <utils/result.h>

#include <QJsonArray>
#include <QJsonObject>

#ifdef WITH_TESTS
#   include <QTest>
#endif // WITH_TESTS

using namespace ProjectExplorer;
using namespace QtSupport;

namespace Android::Internal {

static QJsonArray allKitsJson()
{
    QJsonArray kits;
    for (Kit *kit : KitManager::kits()) {
        kits.append(QJsonObject{
            {"id", kit->id().toString()},
            {"name", kit->displayName()},
            {"valid", kit->isValid()},
            {"device_type", RunDeviceTypeKitAspect::deviceTypeId(kit).toString()}});
    }
    return kits;
}

static QJsonArray androidQtVersionsJson()
{
    QJsonArray versions;
    for (const QtVersion *qt : QtVersionManager::versions()) {
        if (qt->type() != Constants::ANDROID_QT_TYPE)
            continue;
        QJsonArray abis;
        for (const Abi &abi : qt->qtAbis())
            abis.append(abi.toString());
        versions.append(QJsonObject{
            {"name", qt->displayName()},
            {"valid", qt->isValid()},
            {"qmake", qt->qmakeFilePath().toUserOutput()},
            {"abis", abis}});
    }
    return versions;
}

static QJsonArray androidToolchainsJson()
{
    QJsonArray tcs;
    const Toolchains all = ToolchainManager::toolchains([](const Toolchain *tc) {
        return tc->typeId() == Utils::Id(Constants::ANDROID_TOOLCHAIN_TYPEID);
    });
    for (const Toolchain *tc : all) {
        tcs.append(QJsonObject{
            {"name", tc->displayName()},
            {"abi", tc->targetAbi().toString()},
            {"valid", tc->isValid()}});
    }
    return tcs;
}

void registerAndroidMcpTools()
{
    using namespace Mcp::Schema;
    using Mcp::ToolRegistry;

    using SimplifiedCallback = std::function<Utils::Result<QJsonObject>(const QJsonObject &)>;
    static const auto wrap = [](const SimplifiedCallback &cb) {
        return [cb](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const Utils::Result<QJsonObject> result = cb(params.argumentsAsObject());
            if (!result)
                return Utils::ResultError(result.error());
            return CallToolResult{}.structuredContent(*result).isError(false);
        };
    };

    ToolRegistry::registerTool(
        Tool{}
            .name("setup_android")
            .title("Set up Android toolchains and kits")
            .description(
                "Runs Android auto-configuration - the same action as applying the Android SDK "
                "preferences page - which registers the NDK toolchains and (re)creates the "
                "automatic Android kits from the configured SDK/NDK and the installed "
                "Qt-for-Android versions. Use after the Android SDK location and a Qt-for-Android "
                "version are configured to obtain a usable Android kit without driving the "
                "preferences GUI. Returns all kits present afterwards, each with the id of its "
                "run device type, so that the Android ones can be told apart, plus the Android "
                "Qt versions and Android toolchains that the kit creation had to work with. "
                "Fails if the configured Android SDK is not usable, which is otherwise "
                "indistinguishable from having no NDK and no Qt-for-Android version.")
            .annotations(ToolAnnotations{}.readOnlyHint(false).idempotentHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "kits",
                        QJsonObject{
                            {"type", "array"},
                            {"items",
                             QJsonObject{
                                 {"type", "object"},
                                 {"properties",
                                  QJsonObject{
                                      {"id", QJsonObject{{"type", "string"}}},
                                      {"name", QJsonObject{{"type", "string"}}},
                                      {"valid", QJsonObject{{"type", "boolean"}}},
                                      {"device_type", QJsonObject{{"type", "string"}}},
                                  }}}},
                            {"description",
                             QString("All kits present after Android setup. An Android kit has "
                                     "\"%1\" as its device_type.")
                                 .arg(QLatin1String(Constants::ANDROID_DEVICE_TYPE))}})
                    .addProperty(
                        "android_qt_versions",
                        QJsonObject{
                            {"type", "array"},
                            {"items",
                             QJsonObject{
                                 {"type", "object"},
                                 {"properties",
                                  QJsonObject{
                                      {"name", QJsonObject{{"type", "string"}}},
                                      {"valid", QJsonObject{{"type", "boolean"}}},
                                      {"qmake", QJsonObject{{"type", "string"}}},
                                      {"abis",
                                       QJsonObject{
                                           {"type", "array"},
                                           {"items", QJsonObject{{"type", "string"}}}}},
                                  }}}},
                            {"description",
                             "Registered Qt-for-Android versions the kit creation drew on"}})
                    .addProperty(
                        "android_toolchains",
                        QJsonObject{
                            {"type", "array"},
                            {"items",
                             QJsonObject{
                                 {"type", "object"},
                                 {"properties",
                                  QJsonObject{
                                      {"name", QJsonObject{{"type", "string"}}},
                                      {"abi", QJsonObject{{"type", "string"}}},
                                      {"valid", QJsonObject{{"type", "boolean"}}},
                                  }}}},
                            {"description", "Android NDK toolchains registered by the setup"}})
                    .addRequired("kits")
                    .addRequired("android_qt_versions")
                    .addRequired("android_toolchains")),
        wrap([](const QJsonObject &) -> Utils::Result<QJsonObject> {
            if (!AndroidConfig::sdkToolsOk()) {
                return Utils::ResultError(
                    QString("The Android SDK at \"%1\" is not usable. Set a writable SDK "
                            "location holding the SDK tools in the Android preferences.")
                        .arg(AndroidConfig::sdkLocation().toUserOutput()));
            }
            AndroidConfigurations::applyConfig();
            return QJsonObject{
                {"kits", allKitsJson()},
                {"android_qt_versions", androidQtVersionsJson()},
                {"android_toolchains", androidToolchainsJson()}};
        }));
}

#ifdef WITH_TESTS

class AndroidMcpSupportTest final : public QObject
{
    Q_OBJECT

private slots:
    void testSetupRejectsUnusableSdk();
};

void AndroidMcpSupportTest::testSetupRejectsUnusableSdk()
{
    const Utils::FilePath original = AndroidConfig::sdkLocation();
    AndroidConfig::setSdkLocation(Utils::FilePath::fromUserInput("/no/such/android/sdk"));
    const Utils::Result<Mcp::Schema::CallToolResult> result
        = Mcp::ToolRegistry::callToolForTests("setup_android", {});
    AndroidConfig::setSdkLocation(original); // Before asserting, so a failure leaves no trace.

    QVERIFY(!result);
    QVERIFY2(result.error().contains("not usable"), qPrintable(result.error()));
}

QObject *createAndroidMcpSupportTest()
{
    return new AndroidMcpSupportTest;
}

#endif // WITH_TESTS

} // namespace Android::Internal

#include "androidmcpsupport.moc"
