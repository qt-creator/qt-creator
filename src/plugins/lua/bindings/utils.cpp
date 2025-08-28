// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"
#include "../luaqttypes.h"

#include <coreplugin/icore.h>

#include "utils.h"

#include <utils/async.h>
#include <utils/commandline.h>
#include <utils/futuresynchronizer.h>
#include <utils/hostosinfo.h>
#include <utils/icon.h>
#include <utils/id.h>
#include <utils/infobar.h>
#include <utils/processinterface.h>

#include <QDesktopServices>
#include <QTimer>
#include <QUuid>

using namespace Utils;
using namespace std::string_view_literals;

namespace Lua::Internal {

void setupUtilsModule()
{
    registerProvider(
        "Utils", [futureSync = FutureSynchronizer()](sol::state_view lua) mutable -> sol::object {

            const ScriptPluginSpec *pluginSpec = lua.get<ScriptPluginSpec *>("PluginSpec"sv);

            auto async = lua.script("return require('async')", "_utils_").get<sol::table>();

            sol::table utils = lua.create_table();

            utils.set_function(
                "waitms_cb",
                [guard = pluginSpec->connectionGuard.get()](int ms, const sol::function &cb) {
                    QTimer::singleShot(ms, guard, [cb]() { cb(); });
                });

            auto dirEntries_cb = [&futureSync, guard = pluginSpec->connectionGuard.get()](
                                     const FilePath &p,
                                     const sol::table &options,
                                     const sol::function &cb) {
                const QStringList nameFilters = options.get_or<QStringList>("nameFilters"sv, {});
                QDir::Filters fileFilters
                    = (QDir::Filters) options.get_or<int>("fileFilters"sv, QDir::NoFilter);
                QDirIterator::IteratorFlags flags
                    = (QDirIterator::IteratorFlags)
                          options.get_or<int>("flags"sv, QDirIterator::NoIteratorFlags);

                FileFilter filter(nameFilters, fileFilters, flags);

                QFuture<FilePath> future = asyncRun([p, filter](QPromise<FilePath> &promise) {
                    p.iterateDirectory(
                        [&promise](const FilePath &item) {
                            if (promise.isCanceled())
                                return IterationPolicy::Stop;

                            promise.addResult(item);
                            return IterationPolicy::Continue;
                        },
                        filter);
                });

                futureSync.addFuture<FilePath>(future);

                onFinished<FilePath>(future, guard, [cb](const QFuture<FilePath> &future) {
                    cb(future.results());
                });
            };

            auto searchInPath_cb = [&futureSync,
                                    guard = pluginSpec->connectionGuard
                                                .get()](const FilePath &p, const sol::function &cb) {
                QFuture<FilePath> future = asyncRun(
                    [p](QPromise<FilePath> &promise) { promise.addResult(p.searchInPath()); });

                futureSync.addFuture<FilePath>(future);

                onFinished<FilePath>(future, guard, [cb](const QFuture<FilePath> &future) {
                    cb(future.result());
                });
            };

            utils.set_function("__dirEntries_cb__", dirEntries_cb);
            utils.set_function("__searchInPath_cb__", searchInPath_cb);

            utils.set_function("createUuid", []() { return QUuid::createUuid().toString(); });

            utils.set_function("getNativeShortcut", [](QString shortcut) {
                return QKeySequence::fromString(shortcut).toString(QKeySequence::NativeText);
            });

            sol::function wrap = async["wrap"].get<sol::function>();

            utils["waitms"] = wrap(utils["waitms_cb"]);

            utils["pid"] = QCoreApplication::applicationPid();

            utils.new_usertype<Utils::Id>(
                "Id",
                sol::no_constructor);

            auto hostOsInfoType = utils.new_usertype<HostOsInfo>("HostOsInfo");
            hostOsInfoType["isWindowsHost"] = &HostOsInfo::isWindowsHost;
            hostOsInfoType["isMacHost"] = &HostOsInfo::isMacHost;
            hostOsInfoType["isLinuxHost"] = &HostOsInfo::isLinuxHost;
            hostOsInfoType["os"] = sol::var([]() {
                if (HostOsInfo::isMacHost())
                    return "mac";
                else if (HostOsInfo::isLinuxHost())
                    return "linux";
                else if (HostOsInfo::isWindowsHost())
                    return "windows";
                else
                    return "unknown";
            }());
            hostOsInfoType["architecture"] = sol::var([]() {
                switch (HostOsInfo::hostArchitecture()) {
                case OsArchUnknown:
                    return "unknown";
                case OsArchX86:
                    return "x86";
                case OsArchAMD64:
                    return "x86_64";
                case OsArchItanium:
                    return "itanium";
                case OsArchArm:
                    return "arm";
                case OsArchArm64:
                    return "arm64";
                default:
                    return "unknown";
                }
            }());

            sol::usertype<FilePath> filePathType = utils.new_usertype<FilePath>(
                "FilePath",
                sol::call_constructor,
                sol::constructors<FilePath()>(),
                sol::meta_function::to_string,
                &FilePath::toUserOutput,
                "fromUserInput",
                &FilePath::fromUserInput,
                "exists",
                &FilePath::exists,
                "resolveSymlinks",
                &FilePath::resolveSymlinks,
                "isExecutableFile",
                &FilePath::isExecutableFile,
                "isDir",
                &FilePath::isDir,
                "nativePath",
                &FilePath::nativePath,
                "toUserOutput",
                &FilePath::toUserOutput,
                "fileName",
                &FilePath::fileName,
                "currentWorkingPath",
                &FilePath::currentWorkingPath,
                "parentDir",
                &FilePath::parentDir,
                "suffix",
                &FilePath::suffix,
                "completeSuffix",
                &FilePath::completeSuffix,
                "isAbsolutePath",
                &FilePath::isAbsolutePath,
                "resolvePath",
                sol::overload(
                    [](const FilePath &p, const QString &path) { return p.resolvePath(path); },
                    [](const FilePath &p, const FilePath &path) { return p.resolvePath(path); }),
                "permissions",
                [](FilePath& p) {
                    return static_cast<QFileDevice::Permission>(p.permissions().toInt());
                },
                "setPermissions",
                [](FilePath& p, QFileDevice::Permission permissions) {
                    p.setPermissions(static_cast<QFile::Permissions>(permissions));
                });

            utils["FilePath"]["dirEntries_cb"] = utils["__dirEntries_cb__"];
            utils["FilePath"]["dirEntries"] = wrap(utils["__dirEntries_cb__"]);

            utils["FilePath"]["searchInPath_cb"] = utils["__searchInPath_cb__"];
            utils["FilePath"]["searchInPath"] = wrap(utils["__searchInPath_cb__"]);

            utils["standardLocations"] = [](QStandardPaths::StandardLocation location) {
                const auto locationsStrings = QStandardPaths::standardLocations(
                    static_cast<QStandardPaths::StandardLocation>(location));

                QList<FilePath> locationsPaths;
                std::transform(locationsStrings.constBegin(), locationsStrings.constEnd(),
                               std::back_inserter(locationsPaths), &FilePath::fromString);
                return locationsPaths;
            };
            utils["standardLocation"] =
                [](QStandardPaths::StandardLocation location) -> sol::optional<FilePath> {
                const auto paths = QStandardPaths::standardLocations(
                    static_cast<QStandardPaths::StandardLocation>(location));
                if (paths.isEmpty())
                    return sol::nullopt;

                return FilePath::fromString(paths.first());
            };
            utils["writableLocation"] =
                [](QStandardPaths::StandardLocation location) -> sol::optional<FilePath> {
                const auto path = QStandardPaths::writableLocation(
                    static_cast<QStandardPaths::StandardLocation>(location));
                if (path.isEmpty())
                    return sol::nullopt;

                return FilePath::fromString(path);
            };

            utils.new_usertype<CommandLine>(
                "CommandLine",
                sol::call_constructor,
                sol::constructors<CommandLine()>(),
                "executable",
                sol::property(&CommandLine::executable, &CommandLine::setExecutable),
                "arguments",
                sol::property(&CommandLine::arguments, &CommandLine::setArguments),
                "addArgument",
                [](CommandLine &cmd, const QString &arg) { cmd.addArg(arg); },
                sol::meta_function::to_string,
                &CommandLine::toUserOutput);

            auto procRunDataType = utils.new_usertype<ProcessRunData>(
                "ProcessRunData",
                sol::call_constructor,
                sol::constructors<ProcessRunData()>(),
                "commandLine",
                sol::property(
                    [](const ProcessRunData &prd) { return prd.command; },
                    [](ProcessRunData &prd, const CommandLine &cmd) { prd.command = cmd; }),
                "workingDirectory",
                sol::property(
                    [](const ProcessRunData &prd) { return prd.workingDirectory; },
                    [](ProcessRunData &prd, const FilePath &wd) { prd.workingDirectory = wd; }),
                "environment",
                sol::property(
                    [](const ProcessRunData &prd) { return prd.environment; },
                    [](ProcessRunData &prd, const Environment &env) { prd.environment = env; }),
                sol::meta_function::to_string,
                [](const ProcessRunData &prd) {
                    return QString("ProcessRunData{\n  command=%1,\n  workingDirectory=%2,\n  "
                                   "environment={\n    %3\n}\n}")
                        .arg(prd.command.toUserOutput())
                        .arg(prd.workingDirectory.toUrlishString())
                        .arg(prd.environment.toStringList().join(",\n    "));
                });

            utils.new_usertype<QTimer>(
                "Timer",
                "create",
                [guard = pluginSpec](int timeout, bool singleShort, sol::main_function callback)
                    -> std::unique_ptr<QTimer> {
                    auto timer = std::make_unique<QTimer>();
                    timer->setInterval(timeout);
                    timer->setSingleShot(singleShort);
                    QObject::connect(
                        timer.get(), &QTimer::timeout, guard->connectionGuard.get(), [callback] {
                            void_safe_call(callback);
                        });

                    return timer;
                },
                "start",
                [](QTimer *timer) { timer->start(); },
                "stop",
                [](QTimer *timer) { timer->stop(); });

            utils["openExternalUrl"] = [](const QString &url) {
                QDesktopServices::openUrl(QUrl::fromEncoded(url.toUtf8()));
            };

            utils["stringToBase64Url"] = [](const QString &data) {
                return QString::fromLatin1(data.toUtf8().toBase64(QByteArray::Base64UrlEncoding));
            };
            utils["base64UrlToString"] = [](const char *data) {
                return QString::fromUtf8(
                    QByteArray::fromBase64(data, QByteArray::Base64UrlEncoding));
            };

            utils["stringToBase64"] = [](const QString &data) {
                return QString::fromLatin1(data.toUtf8().toBase64());
            };
            utils["base64ToString"] = [](const char *data) {
                return QString::fromUtf8(QByteArray::fromBase64(data));
            };

            utils.new_enum<Icon::IconStyleOption>(
                "IconStyleOption",
                {{"None", Icon::None},
                 {"Tint", Icon::Tint},
                 {"DropShadow", Icon::DropShadow},
                 {"PunchEdges", Icon::PunchEdges},
                 {"ToolBarStyle", Icon::ToolBarStyle},
                 {"MenuTintedStyle", Icon::MenuTintedStyle}});

            mirrorEnum(utils, QMetaEnum::fromType<Theme::Color>(), "ThemeColor");

            auto iconType = utils.new_usertype<Icon>(
                "Icon",
                "create",
                sol::factories(
                    [](FilePathOrString path) { return std::make_shared<Icon>(toFilePath(path)); },
                    [](const sol::table &maskAndColor, Icon::IconStyleOption style) {
                        QList<IconMaskAndColor> args;
                        for (const auto &it : maskAndColor) {
                            auto pair = it.second.as<sol::table>();
                            args.append(qMakePair(
                                toFilePath(pair.get<FilePathOrString>(1)),
                                pair.get<Theme::Color>(2)));
                        }
                        return std::make_shared<Icon>(args, Icon::IconStyleOptions::fromInt(style));
                    }));

            return utils;
        });
}

InfoBarCleaner::~InfoBarCleaner()
{
    for (const auto &id : std::as_const(openInfoBars))
        Core::ICore::infoBar()->removeInfo(id);
}

} // namespace Lua::Internal
