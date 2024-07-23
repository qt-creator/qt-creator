// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"
#include "../luaqttypes.h"
#include "../luatr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/aspects.h>
#include <utils/infobar.h>
#include <utils/layoutbuilder.h>
#include <utils/networkaccessmanager.h>
#include <utils/stylehelper.h>

#include <QApplication>
#include <QCheckBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QMetaEnum>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Lua::Internal {

static QString opToString(QNetworkAccessManager::Operation op)
{
    switch (op) {
    case QNetworkAccessManager::HeadOperation:
        return "HEAD";
    case QNetworkAccessManager::GetOperation:
        return "GET";
    case QNetworkAccessManager::PutOperation:
        return "PUT";
    case QNetworkAccessManager::PostOperation:
        return "POST";

    case QNetworkAccessManager::DeleteOperation:
        return "DELETE";
    case QNetworkAccessManager::CustomOperation:
        return "CUSTOM";
    default:
        return "UNKNOWN";
    }
}

void addFetchModule()
{
    class Module : Utils::AspectContainer
    {
        Utils::StringListAspect pluginsAllowedToFetch{this};
        Utils::StringListAspect pluginsNotAllowedToFetch{this};

        class LuaOptionsPage : public Core::IOptionsPage
        {
        public:
            LuaOptionsPage(Module *module)
            {
                setId("BB.Lua.Fetch");
                setDisplayName(Tr::tr("Network Access"));
                setCategory("ZY.Lua");
                setDisplayCategory("Lua");
                setCategoryIconPath(":/lua/images/settingscategory_lua.png");
                setSettingsProvider(
                    [module] { return static_cast<Utils::AspectContainer *>(module); });
            }
        };

        LuaOptionsPage settingsPage{this};

    public:
        Module()
        {
            setSettingsGroup("Lua.Fetch");

            pluginsAllowedToFetch.setSettingsKey("pluginsAllowedToFetch");
            pluginsAllowedToFetch.setLabelText("Plugins allowed to fetch data from the internet");
            pluginsAllowedToFetch.setToolTip(
                "List of plugins that are allowed to fetch data from the internet");
            pluginsAllowedToFetch.setUiAllowAdding(false);
            pluginsAllowedToFetch.setUiAllowEditing(false);

            pluginsNotAllowedToFetch.setSettingsKey("pluginsNotAllowedToFetch");
            pluginsNotAllowedToFetch.setLabelText(
                "Plugins not allowed to fetch data from the internet");
            pluginsNotAllowedToFetch.setToolTip(
                "List of plugins that are not allowed to fetch data from the internet");
            pluginsNotAllowedToFetch.setUiAllowAdding(false);
            pluginsNotAllowedToFetch.setUiAllowEditing(false);

            setLayouter([this] {
                using namespace Layouting;
                // clang-format off
                return Form {
                    pluginsAllowedToFetch, br,
                    pluginsNotAllowedToFetch, br,
                };
                // clang-format on
            });

            readSettings();
        }

        ~Module() { writeSettings(); }

        enum class IsAllowed { Yes, No, NeedsToAsk };

        IsAllowed isAllowedToFetch(const QString &pluginName) const
        {
            if (pluginsAllowedToFetch().contains(pluginName))
                return IsAllowed::Yes;
            if (pluginsNotAllowedToFetch().contains(pluginName))
                return IsAllowed::No;
            return IsAllowed::NeedsToAsk;
        }

        void setAllowedToFetch(const QString &pluginName, IsAllowed allowed)
        {
            if (allowed == IsAllowed::Yes)
                pluginsAllowedToFetch.appendValue(pluginName);
            else if (allowed == IsAllowed::No)
                pluginsNotAllowedToFetch.appendValue(pluginName);

            if (allowed == IsAllowed::Yes)
                pluginsNotAllowedToFetch.removeValue(pluginName);
            else if (allowed == IsAllowed::No)
                pluginsAllowedToFetch.removeValue(pluginName);
        }
    };

    std::shared_ptr<Module> module = std::make_shared<Module>();

    LuaEngine::registerProvider("Fetch", [mod = std::move(module)](sol::state_view lua) -> sol::object {
        const ScriptPluginSpec *pluginSpec = lua.get<ScriptPluginSpec *>("PluginSpec");

        sol::table async = lua.script("return require('async')", "_fetch_").get<sol::table>();
        sol::function wrap = async["wrap"];

        sol::table fetch = lua.create_table();

        auto networkReplyType = lua.new_usertype<QNetworkReply>(
            "QNetworkReply",
            "error",
            sol::property([](QNetworkReply *self) -> int { return self->error(); }),
            "readAll",
            [](QNetworkReply *r) { return r->readAll().toStdString(); },
            "__tostring",
            [](QNetworkReply *r) {
                return QString("QNetworkReply(%1 \"%2\") => %3")
                    .arg(opToString(r->operation()))
                    .arg(r->url().toString())
                    .arg(r->error());
            });

        auto checkPermission = [mod,
                                pluginName = pluginSpec->name,
                                guard = pluginSpec->connectionGuard.get()](
                                   QString url,
                                   std::function<void()> fetch,
                                   std::function<void()> notAllowed) {
            auto isAllowed = mod->isAllowedToFetch(pluginName);
            if (isAllowed == Module::IsAllowed::Yes) {
                fetch();
                return;
            }

            if (isAllowed == Module::IsAllowed::No) {
                notAllowed();
                return;
            }

            if (QApplication::activeModalWidget()) {
                // We are already showing a modal dialog,
                // so we have to use a QMessageBox instead of the info bar
                auto msgBox = new QMessageBox(
                    QMessageBox::Question,
                    Tr::tr("Allow Internet Access"),
                    Tr::tr("Allow the extension \"%1\" to fetch from the following URL:\n%2")
                        .arg(pluginName)
                        .arg(url),
                    QMessageBox::Yes | QMessageBox::No,
                    Core::ICore::dialogParent());
                msgBox->setCheckBox(new QCheckBox(Tr::tr("Remember choice")));

                QObject::connect(
                    msgBox, &QMessageBox::accepted, guard, [mod, fetch, pluginName, msgBox]() {
                        if (msgBox->checkBox()->isChecked())
                            mod->setAllowedToFetch(pluginName, Module::IsAllowed::Yes);
                        fetch();
                    });

                QObject::connect(
                    msgBox, &QMessageBox::rejected, guard, [mod, notAllowed, pluginName, msgBox]() {
                        if (msgBox->checkBox()->isChecked())
                            mod->setAllowedToFetch(pluginName, Module::IsAllowed::No);
                        notAllowed();
                    });

                msgBox->show();

                return;
            }

            Utils::InfoBarEntry entry{
                Utils::Id("Fetch").withSuffix(pluginName),
                Tr::tr("Allow the extension \"%1\" to fetch data from the internet?")
                    .arg(pluginName)};
            entry.setDetailsWidgetCreator([pluginName, url] {
                const QString markdown = Tr::tr("Allow the extension \"%1\" to fetch data"
                                                "from the following URL:\n\n")
                                             .arg("**" + pluginName + "**")
                                         + QString("* [%1](%1)").arg(url);

                QLabel *list = new QLabel();
                list->setTextFormat(Qt::TextFormat::MarkdownText);
                list->setText(markdown);
                list->setMargin(Utils::StyleHelper::SpacingTokens::ExPaddingGapS);
                return list;
            });
            entry.addCustomButton(Tr::tr("Always Allow"), [mod, pluginName, fetch]() {
                mod->setAllowedToFetch(pluginName, Module::IsAllowed::Yes);
                Core::ICore::infoBar()->removeInfo(Utils::Id("Fetch").withSuffix(pluginName));
                fetch();
            });
            entry.addCustomButton(Tr::tr("Allow Once"), [pluginName, fetch]() {
                Core::ICore::infoBar()->removeInfo(Utils::Id("Fetch").withSuffix(pluginName));
                fetch();
            });

            entry.setCancelButtonInfo(Tr::tr("Deny"), [mod, notAllowed, pluginName]() {
                Core::ICore::infoBar()->removeInfo(Utils::Id("Fetch").withSuffix(pluginName));
                mod->setAllowedToFetch(pluginName, Module::IsAllowed::No);
                notAllowed();
            });
            Core::ICore::infoBar()->addInfo(entry);
        };

        fetch["fetch_cb"] = [checkPermission,
                             pluginName = pluginSpec->name,
                             guard = pluginSpec->connectionGuard.get(),
                             mod](
                                const sol::table &options,
                                const sol::function &callback,
                                const sol::this_state &thisState) {
            auto url = options.get<QString>("url");
            auto actualFetch = [guard, url, options, callback, thisState]() {
                auto method = (options.get_or<QString>("method", "GET")).toLower();
                auto headers = options.get_or<sol::table>("headers", {});
                auto data = options.get_or<QString>("body", {});
                bool convertToTable
                    = options.get<std::optional<bool>>("convertToTable").value_or(false);

                QNetworkRequest request((QUrl(url)));
                if (headers && !headers.empty()) {
                    for (const auto &[k, v] : headers)
                        request.setRawHeader(k.as<QString>().toUtf8(), v.as<QString>().toUtf8());
                }

                QNetworkReply *reply = nullptr;
                if (method == "get")
                    reply = Utils::NetworkAccessManager::instance()->get(request);
                else if (method == "post")
                    reply = Utils::NetworkAccessManager::instance()->post(request, data.toUtf8());
                else
                    throw std::runtime_error("Unknown method: " + method.toStdString());

                if (convertToTable) {
                    QObject::connect(
                        reply, &QNetworkReply::finished, guard, [reply, thisState, callback]() {
                            reply->deleteLater();

                            if (reply->error() != QNetworkReply::NoError) {
                                callback(QString("%1 (%2):\n%3")
                                             .arg(reply->errorString())
                                             .arg(QString::fromLatin1(
                                                 QMetaEnum::fromType<QNetworkReply::NetworkError>()
                                                     .valueToKey(reply->error())))
                                             .arg(QString::fromUtf8(reply->readAll())));
                                return;
                            }

                            QByteArray data = reply->readAll();
                            QJsonParseError error;
                            QJsonDocument doc = QJsonDocument::fromJson(data, &error);
                            if (error.error != QJsonParseError::NoError) {
                                callback(error.errorString());
                                return;
                            }

                            callback(LuaEngine::toTable(thisState, doc));
                        });

                } else {
                    QObject::connect(
                        reply, &QNetworkReply::finished, guard, [reply, callback]() {
                            // We don't want the network reply to be deleted by the manager, but
                            // by the Lua GC
                            reply->setParent(nullptr);
                            callback(std::unique_ptr<QNetworkReply>(reply));
                        });
                }
            };

            checkPermission(url, actualFetch, [callback, pluginName]() {
                callback(Tr::tr("Fetching is not allowed for the extension \"%1\". (You can edit "
                                "permissions in Preferences > Lua.)")
                             .arg(pluginName));
            });
        };

        fetch["fetch"] = wrap(fetch["fetch_cb"]);

        return fetch;
    });
}

} // namespace Lua::Internal
