// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"
#include "../luatr.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/taskprogress.h>

#include <solutions/tasking/networkquery.h>
#include <solutions/tasking/tasktree.h>

#include <utils/algorithm.h>
#include <utils/infobar.h>
#include <utils/networkaccessmanager.h>
#include <utils/stylehelper.h>
#include <utils/unarchiver.h>

#include <QApplication>
#include <QJsonDocument>
#include <QLabel>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QtConcurrent>

using namespace Core;
using namespace Tasking;
using namespace Utils;

namespace Lua::Internal {

expected_str<QJsonDocument> getPackageInfo(const FilePath &appDataPath)
{
    const FilePath packageInfoPath = appDataPath / "package.json";

    if (!packageInfoPath.exists())
        return QJsonDocument();

    expected_str<QByteArray> json = packageInfoPath.fileContents();
    if (!json)
        return make_unexpected(json.error());

    if (json->isEmpty())
        return QJsonDocument();

    QJsonParseError error;
    QJsonDocument doc(QJsonDocument::fromJson(*json, &error));
    if (error.error != QJsonParseError::NoError)
        return make_unexpected(error.errorString());

    if (!doc.isObject())
        return make_unexpected(Tr::tr("Package info is not an object."));

    return doc;
}

expected_str<QJsonObject> getInstalledPackageInfo(const FilePath &appDataPath, const QString &name)
{
    auto packageDoc = getPackageInfo(appDataPath);
    if (!packageDoc)
        return make_unexpected(packageDoc.error());

    QJsonObject root = packageDoc->object();

    if (root.contains(name)) {
        QJsonValue v = root[name];
        if (!v.isObject())
            return make_unexpected(Tr::tr("Installed package info is not an object."));
        return v.toObject();
    }

    return QJsonObject();
}

expected_str<QJsonDocument> getOrCreatePackageInfo(const FilePath &appDataPath)
{
    expected_str<QJsonDocument> doc = getPackageInfo(appDataPath);
    if (doc && doc->isObject())
        return doc;

    QJsonObject obj;
    return QJsonDocument(obj);
}

expected_str<void> savePackageInfo(const FilePath &appDataPath, const QJsonDocument &doc)
{
    if (!appDataPath.ensureWritableDir())
        return make_unexpected(Tr::tr("Cannot create app data directory."));

    const FilePath packageInfoPath = appDataPath / "package.json";
    return packageInfoPath.writeFileContents(doc.toJson())
        .transform_error([](const QString &error) {
            return Tr::tr("Cannot write to package info: %1").arg(error);
        })
        .transform([](qint64) { return; });
}

struct InstallOptions
{
    QUrl url;
    QString name;
    QString version;
};

size_t qHash(const InstallOptions &item, size_t seed = 0)
{
    return qHash(item.url, seed) ^ qHash(item.name, seed) ^ qHash(item.version, seed);
}

static FilePath destination(const FilePath &appDataPath, const InstallOptions &options)
{
    return appDataPath / "packages" / options.name / options.version;
}

static Group installRecipe(
    const FilePath &appDataPath,
    const QList<InstallOptions> &installOptions,
    const sol::protected_function &callback)
{
    Storage<QFile> storage;

    const LoopList<InstallOptions> installOptionsIt(installOptions);

    const auto emitResult = [callback](const QString &error = QString()) {
        if (error.isEmpty()) {
            LuaEngine::void_safe_call(callback, true);
            return DoneResult::Success;
        }
        LuaEngine::void_safe_call(callback, false, error);
        return DoneResult::Error;
    };

    const auto onDownloadSetup = [installOptionsIt](NetworkQuery &query) {
        query.setRequest(QNetworkRequest(installOptionsIt->url));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
        return SetupResult::Continue;
    };

    const auto onDownloadDone = [emitResult, storage](const NetworkQuery &query, DoneWith result) {
        if (result == DoneWith::Error)
            return emitResult(query.reply()->errorString());
        if (result == DoneWith::Cancel)
            return DoneResult::Error;

        QNetworkReply *reply = query.reply();
        const auto size = reply->size();
        const auto written = storage->write(reply->readAll());
        if (written != size)
            return emitResult(Tr::tr("Cannot write to temporary file."));
        storage->close();
        return DoneResult::Success;
    };

    const auto onUnarchiveSetup =
        [appDataPath, installOptionsIt, storage, emitResult](Unarchiver &unarchiver) {
            const auto sourceAndCommand = Unarchiver::sourceAndCommand(
                FilePath::fromUserInput(storage->fileName()));

            if (!sourceAndCommand) {
                emitResult(sourceAndCommand.error());
                return SetupResult::StopWithError;
            }
            unarchiver.setGZipFileDestName(installOptionsIt->name);
            unarchiver.setSourceAndCommand(*sourceAndCommand);
            unarchiver.setDestDir(destination(appDataPath, *installOptionsIt));
            return SetupResult::Continue;
        };

    const auto onUnarchiverDone = [appDataPath, installOptionsIt, emitResult](DoneWith result) {
        if (result == DoneWith::Error)
            return emitResult(Tr::tr("Unarchiving failed."));
        if (result == DoneWith::Cancel)
            return DoneResult::Error;

        const FilePath destDir = destination(appDataPath, *installOptionsIt);
        const FilePath binary = destDir / installOptionsIt->name;

        if (binary.isFile())
            binary.setPermissions(QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);

        expected_str<QJsonDocument> doc = getOrCreatePackageInfo(appDataPath);
        if (!doc)
            return emitResult(doc.error());

        QJsonObject obj = doc->object();
        QJsonObject installedPackage;
        installedPackage["version"] = installOptionsIt->version;
        installedPackage["name"] = installOptionsIt->name;
        installedPackage["path"] = destDir.toFSPathString();
        obj[installOptionsIt->name] = installedPackage;

        expected_str<void> res = savePackageInfo(appDataPath, QJsonDocument(obj));
        if (!res)
            return emitResult(res.error());
        return DoneResult::Success;
    };

    return For {
        installOptionsIt,
        storage,
        parallelIdealThreadCountLimit,
        Group{
            onGroupSetup([emitResult, storage, installOptionsIt] {
                const QString fileName = installOptionsIt->url.fileName();
                const QString ext = fileName.mid(fileName.indexOf('.'));
                {
                    QTemporaryFile tempFile(QDir::tempPath() + "/XXXXXX" + ext);
                    tempFile.setAutoRemove(false);
                    tempFile.open();
                    (*storage).setFileName(tempFile.fileName());
                }

                if (!storage->open(QIODevice::WriteOnly)) {
                    emitResult(Tr::tr("Cannot open temporary file."));
                    return SetupResult::StopWithError;
                }
                return SetupResult::Continue;
            }),
            NetworkQueryTask(onDownloadSetup, onDownloadDone),
            UnarchiverTask(onUnarchiveSetup, onUnarchiverDone),
            onGroupDone([storage, emitResult] { storage->remove(); }),
        },
        onGroupDone([emitResult](DoneWith result) {
            if (result == DoneWith::Cancel)
                emitResult("Installation was canceled");
            else if (result == DoneWith::Success)
                emitResult();
        }),
    };
}

void addInstallModule()
{
    class State
    {
    public:
        State() = default;
        State(const State &) {}
        ~State()
        {
            for (auto tree : m_trees)
                delete tree;
        }

        TaskTree *createTree()
        {
            auto tree = new TaskTree();
            m_trees.append(tree);
            QObject::connect(tree, &TaskTree::done, tree, &QObject::deleteLater);
            return tree;
        };

    private:
        QList<QPointer<TaskTree>> m_trees;
    };

    LuaEngine::registerProvider(
        "Install", [state = State()](sol::state_view lua) mutable -> sol::object {
            sol::table async
                = lua.script("return require('async')", "_install_async_").get<sol::table>();
            sol::function wrap = async["wrap"];

            sol::table install = lua.create_table();
            const ScriptPluginSpec *pluginSpec = lua.get<ScriptPluginSpec *>("PluginSpec");

            install["packageInfo"] =
                [pluginSpec](const QString &name, sol::this_state l) -> sol::optional<sol::table> {
                expected_str<QJsonObject> obj
                    = getInstalledPackageInfo(pluginSpec->appDataPath, name);
                if (!obj)
                    throw sol::error(obj.error().toStdString());

                return sol::table::create_with(
                    l.lua_state(),
                    "name",
                    obj->value("name").toString(),
                    "version",
                    obj->value("version").toString(),
                    "path",
                    FilePath::fromUserInput(obj->value("path").toString()));
            };

            install["install_cb"] =
                [pluginSpec, &state](
                    const QString &msg,
                    const sol::table &installOptions,
                    const sol::function &callback) {
                    QList<InstallOptions> installOptionsList;
                    if (installOptions.size() > 0) {
                        for (const auto &pair : installOptions) {
                            const sol::object &value = pair.second;
                            if (value.get_type() != sol::type::table)
                                throw sol::error("Install options must be a table");
                            const sol::table &table = value.as<sol::table>();
                            const QString name = table["name"];
                            const QUrl url = QUrl::fromUserInput(table["url"]);
                            if (url.scheme() != "https")
                                throw sol::error("Only HTTPS is supported");

                            const QString version = table["version"];
                            installOptionsList.append({url, name, version});
                        }
                    } else {
                        const QString name = installOptions["name"];
                        const QUrl url = QUrl::fromUserInput(installOptions["url"]);
                        const QString version = installOptions["version"];
                        if (url.scheme() != "https")
                            throw sol::error("Only HTTPS is supported");
                        installOptionsList.append({url, name, version});
                    }

                    auto install = [&state, pluginSpec, installOptionsList, callback]() {
                        auto tree = state.createTree();

                        auto progress = new TaskProgress(tree);
                        progress->setDisplayName(Tr::tr("Installing package(s) %1").arg("..."));

                        tree->setRecipe(
                            installRecipe(pluginSpec->appDataPath, installOptionsList, callback));
                        tree->start();
                    };

                    auto denied = [callback]() { callback(false, "User denied installation"); };

                    if (QApplication::activeModalWidget()) {
                        auto msgBox = new QMessageBox(
                            QMessageBox::Question,
                            Tr::tr("Install Package"),
                            msg,
                            QMessageBox::Yes | QMessageBox::No,
                            Core::ICore::dialogParent());

                        const QString details
                            = Tr::tr("The extension \"%1\" wants to install the following "
                                     "package(s):\n\n")
                                  .arg(pluginSpec->name)
                              + Utils::transform(installOptionsList, [](const InstallOptions &options) {
                                    //: %1 = package name, %2 = version, %3 = URL
                                    return QString("* %1 - %2 (from: %3)")
                                        .arg(options.name, options.version, options.url.toString());
                                }).join("\n");

                        msgBox->setDetailedText(details);

                        auto guard = pluginSpec->connectionGuard.get();
                        QObject::connect(msgBox, &QMessageBox::accepted, guard, install);
                        QObject::connect(msgBox, &QMessageBox::rejected, guard, denied);

                        msgBox->show();
                        return;
                    }

                    const Utils::Id infoBarId = Utils::Id::fromString(
                        "Install" + pluginSpec->name + QString::number(qHash(installOptionsList)));

                    InfoBarEntry entry(infoBarId, msg, InfoBarEntry::GlobalSuppression::Enabled);

                    entry.addCustomButton(Tr::tr("Install"), [install, infoBarId]() {
                        install();
                        Core::ICore::infoBar()->removeInfo(infoBarId);
                    });

                    entry.setCancelButtonInfo(denied);

                    const QString details
                        = Tr::tr("The extension \"%1\" wants to install the following "
                                 "package(s):\n\n")
                              .arg("**" + pluginSpec->name + "**") // markdown bold
                          + Utils::transform(installOptionsList, [](const InstallOptions &options) {
                                //: Markdown list item: %1 = package name, %2 = version, %3 = URL
                                return Tr::tr("* %1 - %2 (from: [%3](%3))")
                                    .arg(options.name, options.version, options.url.toString());
                            }).join("\n");

                    entry.setDetailsWidgetCreator([details]() -> QWidget * {
                        QLabel *list = new QLabel();
                        list->setTextFormat(Qt::TextFormat::MarkdownText);
                        list->setText(details);
                        list->setMargin(StyleHelper::SpacingTokens::ExPaddingGapS);
                        return list;
                    });
                    Core::ICore::infoBar()->addInfo(entry);
                };

            install["install"] = wrap(install["install_cb"]);

            return install;
        });
}

} // namespace Lua::Internal
