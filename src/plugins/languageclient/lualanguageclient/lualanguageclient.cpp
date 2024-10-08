// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <coreplugin/editormanager/editormanager.h>

#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientsettings.h>
#include <languageclient/languageclienttr.h>

#include <lua/bindings/inheritance.h>
#include <lua/luaengine.h>

#include <lua/bindings/async.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/commandline.h>
#include <utils/guardedcallback.h>
#include <utils/layoutbuilder.h>

#include <QJsonDocument>

using namespace Utils;
using namespace Core;
using namespace TextEditor;
using namespace ProjectExplorer;

namespace {

class RequestWithResponse : public LanguageServerProtocol::JsonRpcMessage
{
    sol::function m_callback;
    LanguageServerProtocol::MessageId m_id;

public:
    RequestWithResponse(const QJsonObject &obj, const sol::function &cb)
        : LanguageServerProtocol::JsonRpcMessage(obj)
        , m_callback(cb)
    {
        m_id = LanguageServerProtocol::MessageId(obj["id"]);
    }

    std::optional<LanguageServerProtocol::ResponseHandler> responseHandler() const override
    {
        if (!m_id.isValid()) {
            qWarning() << "Invalid 'id' in request:" << toJsonObject();
            return std::nullopt;
        }

        return LanguageServerProtocol::ResponseHandler{
            m_id, [callback = m_callback](const JsonRpcMessage &msg) {
                if (!callback.valid()) {
                    qWarning() << "Invalid Lua callback";
                    return;
                }

                auto result = ::Lua::void_safe_call(
                    callback, ::Lua::toTable(callback.lua_state(), msg.toJsonObject()));
                QTC_CHECK_EXPECTED(result);
            }};
    }
};

} // anonymous namespace

namespace LanguageClient::Lua {

static void registerLuaApi();

class LuaClient : public LanguageClient::Client
{
    Q_OBJECT

public:
    Utils::Id m_settingsId;

    LuaClient(BaseClientInterface *interface, Utils::Id settingsId)
        : LanguageClient::Client(interface)
        , m_settingsId(settingsId)
    {}
};

class LuaLanguageClientPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "LuaLanguageClient.json")

public:
    LuaLanguageClientPlugin() {}

private:
    void initialize() final { registerLuaApi(); }
};

class LuaLocalSocketClientInterface : public LocalSocketClientInterface
{
public:
    LuaLocalSocketClientInterface(const CommandLine &cmd, const QString &serverName)
        : LocalSocketClientInterface(serverName)
        , m_cmd(cmd)
        , m_logFile("lua-lspclient.XXXXXX.log")

    {}

    void startImpl() override
    {
        if (m_process) {
            QTC_CHECK(!m_process->isRunning());
            delete m_process;
        }
        m_process = new Process;
        m_process->setProcessMode(ProcessMode::Writer);
        connect(
            m_process,
            &Process::readyReadStandardError,
            this,
            &LuaLocalSocketClientInterface::readError);
        connect(
            m_process,
            &Process::readyReadStandardOutput,
            this,
            &LuaLocalSocketClientInterface::readOutput);
        connect(m_process, &Process::started, this, [this]() {
            this->LocalSocketClientInterface::startImpl();
            emit started();
        });
        connect(m_process, &Process::done, this, [this] {
            if (m_process->result() != ProcessResult::FinishedWithSuccess)
                emit error(QString("%1 (see logs in \"%2\")")
                               .arg(m_process->exitMessage())
                               .arg(m_logFile.fileName()));
            emit finished();
        });
        m_logFile.write(
            QString("Starting server: %1\nOutput:\n\n").arg(m_cmd.toUserOutput()).toUtf8());
        m_process->setCommand(m_cmd);
        m_process->setWorkingDirectory(m_workingDirectory);
        if (m_env.hasChanges())
            m_process->setEnvironment(m_env);
        m_process->start();
    }

    void setWorkingDirectory(const FilePath &workingDirectory)
    {
        m_workingDirectory = workingDirectory;
    }

    FilePath serverDeviceTemplate() const override { return m_cmd.executable(); }

    void readError()
    {
        QTC_ASSERT(m_process, return);

        const QByteArray stdErr = m_process->readAllRawStandardError();
        m_logFile.write(stdErr);
    }

    void readOutput()
    {
        QTC_ASSERT(m_process, return);
        const QByteArray &out = m_process->readAllRawStandardOutput();
        parseData(out);
    }

private:
    Utils::CommandLine m_cmd;
    Utils::FilePath m_workingDirectory;
    Utils::Process *m_process = nullptr;
    Utils::Environment m_env;
    QTemporaryFile m_logFile;
};

class LuaClientWrapper;

class LuaClientSettings : public BaseSettings
{
    std::weak_ptr<LuaClientWrapper> m_wrapper;
    QObject guard;

public:
    LuaClientSettings(const LuaClientSettings &wrapper);
    LuaClientSettings(const std::weak_ptr<LuaClientWrapper> &wrapper);
    ~LuaClientSettings() override = default;

    bool applyFromSettingsWidget(QWidget *widget) override;

    Utils::Store toMap() const override;
    void fromMap(const Utils::Store &map) override;

    QWidget *createSettingsWidget(QWidget *parent = nullptr) const override;

    BaseSettings *copy() const override { return new LuaClientSettings(*this); }

protected:
    Client *createClient(BaseClientInterface *interface) const final;

    BaseClientInterface *createInterface(ProjectExplorer::Project *project) const override;
};
enum class TransportType { StdIO, LocalSocket };

class LuaClientWrapper : public QObject
{
    Q_OBJECT
public:
    TransportType m_transportType{TransportType::StdIO};
    std::function<expected_str<void>(CommandLine &)> m_cmdLineCallback;
    std::function<expected_str<void>(QString &)> m_initOptionsCallback;
    sol::function m_asyncInitOptions;
    bool m_isUpdatingAsyncOptions{false};
    AspectContainer *m_aspects{nullptr};
    QString m_name;
    Utils::Id m_settingsTypeId;
    QString m_clientSettingsId;
    QString m_initializationOptions;
    CommandLine m_cmdLine;
    QString m_serverName;
    LanguageFilter m_languageFilter;
    bool m_showInSettings;
    BaseSettings::StartBehavior m_startBehavior = BaseSettings::RequiresFile;

    std::optional<sol::protected_function> m_onInstanceStart;
    std::optional<sol::protected_function> m_startFailedCallback;
    QMap<QString, sol::protected_function> m_messageCallbacks;

public:
    static BaseSettings::StartBehavior startBehaviorFromString(const QString &str)
    {
        if (str == "RequiresProject")
            return BaseSettings::RequiresProject;
        if (str == "RequiresFile")
            return BaseSettings::RequiresFile;
        if (str == "AlwaysOn")
            return BaseSettings::AlwaysOn;

        throw sol::error("Unknown start behavior: " + str.toStdString());
    }

    LuaClientWrapper(const sol::table &options)
    {
        m_cmdLineCallback = addValue<CommandLine>(
            options,
            "cmd",
            m_cmdLine,
            [](const sol::protected_function_result &res) -> expected_str<CommandLine> {
                if (res.get_type(0) != sol::type::table)
                    return make_unexpected(QString("cmd callback did not return a table"));
                return cmdFromTable(res.get<sol::table>());
            });

        m_initOptionsCallback = addValue<QString>(
            options,
            "initializationOptions",
            m_initializationOptions,
            [](const sol::protected_function_result &res) -> expected_str<QString> {
                if (res.get_type(0) == sol::type::table)
                    return ::Lua::toJsonString(res.get<sol::table>());
                else if (res.get_type(0) == sol::type::string)
                    return res.get<QString>(0);
                return make_unexpected(QString("init callback did not return a table or string"));
            });

        if (auto initOptionsTable = options.get<sol::optional<sol::table>>("initializationOptions"))
            m_initializationOptions = ::Lua::toJsonString(*initOptionsTable);
        else if (auto initOptionsString = options.get<sol::optional<QString>>("initializationOptions"))
            m_initializationOptions = *initOptionsString;

        m_name = options.get<QString>("name");
        m_settingsTypeId = Utils::Id::fromString(QString("Lua_%1").arg(m_name));
        m_serverName = options.get_or<QString>("serverName", "");

        m_startBehavior = startBehaviorFromString(
            options.get_or<QString>("startBehavior", "AlwaysOn"));

        m_startFailedCallback = options.get<sol::protected_function>("onStartFailed");

        QString transportType = options.get_or<QString>("transport", "stdio");
        if (transportType == "stdio")
            m_transportType = TransportType::StdIO;
        else if (transportType == "localsocket")
            m_transportType = TransportType::LocalSocket;
        else
            qWarning() << "Unknown transport type:" << transportType;

        auto languageFilter = options.get<std::optional<sol::table>>("languageFilter");
        if (languageFilter) {
            auto patterns = languageFilter->get<std::optional<sol::table>>("patterns");
            auto mimeTypes = languageFilter->get<std::optional<sol::table>>("mimeTypes");

            if (patterns)
                for (auto [_, v] : *patterns)
                    m_languageFilter.filePattern.push_back(v.as<QString>());

            if (mimeTypes)
                for (auto [_, v] : *mimeTypes)
                    m_languageFilter.mimeTypes.push_back(v.as<QString>());
        }

        m_showInSettings = options.get<std::optional<bool>>("showInSettings").value_or(true);

        // get<sol::optional<>> because on MSVC, get_or(..., nullptr) fails to compile
        m_aspects = options.get<sol::optional<AspectContainer *>>("settings").value_or(nullptr);

        if (m_aspects) {
            connect(m_aspects, &AspectContainer::applied, this, [this] {
                updateOptions();
                LanguageClientManager::applySettings();
            });
        }

        connect(
            LanguageClientManager::instance(),
            &LanguageClientManager::clientInitialized,
            this,
            [this](Client *c) {
                auto luaClient = qobject_cast<LuaClient *>(c);
                if (luaClient && luaClient->m_settingsId == m_settingsTypeId && m_onInstanceStart) {
                    QTC_CHECK(::Lua::void_safe_call(*m_onInstanceStart, c));
                    updateMessageCallbacks();
                }
            });

        connect(
            LanguageClientManager::instance(),
            &LanguageClientManager::clientRemoved,
            this,
            &LuaClientWrapper::onClientRemoved);

        if (auto asyncInit = options.get<sol::optional<sol::function>>(
                "initializationOptionsAsync")) {
            m_asyncInitOptions = *asyncInit;
            QMetaObject::invokeMethod(
                this, &LuaClientWrapper::updateAsyncOptions, Qt::QueuedConnection);
        }
    }

    void onClientRemoved(Client *c, bool unexpected)
    {
        auto luaClient = qobject_cast<LuaClient *>(c);
        if (!luaClient || luaClient->m_settingsId != m_settingsTypeId)
            return;

        if (unexpected && m_startFailedCallback) {
            QTC_CHECK_EXPECTED(::Lua::void_safe_call(*m_startFailedCallback));
        }
    }

    TransportType transportType() { return m_transportType; }

    void applySettings()
    {
        if (m_aspects)
            m_aspects->apply();

        updateOptions();
    }

    void fromMap(const Utils::Store &map)
    {
        if (m_aspects)
            m_aspects->fromMap(map);
        updateOptions();
    }

    void toMap(Utils::Store &map) const
    {
        if (m_aspects)
            m_aspects->toMap(map);
    }

    Layouting::LayoutModifier settingsLayout()
    {
        if (m_aspects)
            return [this](Layouting::Layout *iface) { m_aspects->addToLayoutImpl(*iface); };
        return {};
    }

    void registerMessageCallback(const QString &msg, const sol::function &callback)
    {
        m_messageCallbacks.insert(msg, callback);
        updateMessageCallbacks();
    }

    void updateMessageCallbacks()
    {
        for (Client *c : LanguageClientManager::clientsForSettingId(m_clientSettingsId)) {
            if (!c)
                continue;
            for (const auto &[msg, func] : m_messageCallbacks.asKeyValueRange()) {
                c->registerCustomMethod(
                    msg,
                    [self = QPointer<LuaClientWrapper>(this),
                     name = msg](const LanguageServerProtocol::JsonRpcMessage &m) {
                        if (!self)
                            return;

                        auto func = self->m_messageCallbacks.value(name);
                        auto table = ::Lua::toTable(func.lua_state(), m.toJsonObject());
                        auto result = func.call(table);
                        if (!result.valid()) {
                            qWarning() << "Error calling message callback for:" << name << ":"
                                       << (result.get<sol::error>().what());
                        }
                    });
            }
        }
    }

    void sendMessage(const sol::table &message)
    {
        const QJsonValue messageValue = ::Lua::toJson(message);
        if (!messageValue.isObject())
            throw sol::error("Message is not an object");

        const LanguageServerProtocol::JsonRpcMessage request(messageValue.toObject());
        for (Client *c : LanguageClientManager::clientsForSettingId(m_clientSettingsId)) {
            if (c)
                c->sendMessage(request);
        }
    }

    QList<Client *> clientsForDocument(Core::IDocument *document)
    {
        if (m_startBehavior == BaseSettings::RequiresProject) {
            Project *project = ProjectManager::projectForFile(document->filePath());
            const auto clients = LanguageClientManager::clientsForSettingId(m_clientSettingsId);
            return Utils::filtered(clients, [project](Client *c) {
                return c && c->project() == project;
            });
        }

        return LanguageClientManager::clientsForSettingId(m_clientSettingsId);
    }

    void sendMessageForDocument(Core::IDocument *document, const sol::table &message)
    {
        const QJsonValue messageValue = ::Lua::toJson(message);
        if (!messageValue.isObject())
            throw sol::error("Message is not an object");

        const LanguageServerProtocol::JsonRpcMessage request(messageValue.toObject());

        auto clients = clientsForDocument(document);
        QTC_CHECK(clients.size() == 1);

        for (Client *c : clients) {
            if (c)
                c->sendMessage(request);
        }
    }

    void sendMessageWithIdForDocument_cb(
        TextEditor::TextDocument *document, const sol::table &message, const sol::function callback)
    {
        const QJsonValue messageValue = ::Lua::toJson(message);
        if (!messageValue.isObject())
            throw sol::error("Message is not an object");

        QJsonObject obj = messageValue.toObject();
        obj["id"] = QUuid::createUuid().toString();

        const RequestWithResponse request{obj, callback};

        auto clients = clientsForDocument(document);

        QTC_ASSERT(clients.size() != 0, throw sol::error("No client for document found"));
        QTC_ASSERT(clients.size() == 1, throw sol::error("Multiple clients for document found"));
        QTC_ASSERT(clients.front(), throw sol::error("Client is null"));

        clients.front()->sendMessage(request);
    }

    void updateAsyncOptions()
    {
        if (m_isUpdatingAsyncOptions)
            return;
        QTC_ASSERT(m_asyncInitOptions, return);
        m_isUpdatingAsyncOptions = true;
        std::function<void(sol::object)> cb = guardedCallback(this, [this](sol::object options) {
            if (options.is<sol::table>())
                m_initializationOptions = ::Lua::toJsonString(options.as<sol::table>());
            else if (options.is<QString>())
                m_initializationOptions = options.as<QString>();

            emit optionsChanged();
            m_isUpdatingAsyncOptions = false;
        });

        ::Lua::Async::start<sol::object>(m_asyncInitOptions, cb);
    }

    void updateOptions()
    {
        if (m_cmdLineCallback) {
            auto result = m_cmdLineCallback(m_cmdLine);
            if (!result)
                qWarning() << "Error applying option callback:" << result.error();
        }
        if (m_initOptionsCallback) {
            expected_str<void> result = m_initOptionsCallback(m_initializationOptions);
            if (!result)
                qWarning() << "Error applying init option callback:" << result.error();

            // Right now there is only one option that needs to be mirrored to the LSP Settings,
            // so we only emit optionsChanged() here. If another setting should need to be dynamic
            // optionsChanged() needs to be called for it as well, but only once per updateOptions()
            emit optionsChanged();
        }
        if (m_asyncInitOptions)
            updateAsyncOptions();
    }

    static CommandLine cmdFromTable(const sol::table &tbl)
    {
        CommandLine cmdLine;
        cmdLine.setExecutable(FilePath::fromUserInput(tbl.get<QString>(1)));

        for (size_t i = 2; i < tbl.size() + 1; i++)
            cmdLine.addArg(tbl.get<QString>(i));

        return cmdLine;
    }

    template<typename T>
    std::function<expected_str<void>(T &)> addValue(
        const sol::table &options,
        const char *fieldName,
        T &dest,
        std::function<expected_str<T>(const sol::protected_function_result &)> transform)
    {
        auto fixed = options.get<sol::optional<sol::table>>(fieldName);
        auto cb = options.get<sol::optional<sol::protected_function>>(fieldName);

        if (fixed) {
            dest = fixed.value().get<T>(1);
        } else if (cb) {
            std::function<expected_str<void>(T &)> callback =
                [cb, transform](T &dest) -> expected_str<void> {
                auto res = cb.value().call();
                if (!res.valid()) {
                    sol::error err = res;
                    return Utils::make_unexpected(QString::fromLocal8Bit(err.what()));
                }

                expected_str<T> trResult = transform(res);
                if (!trResult)
                    return make_unexpected(trResult.error());

                dest = *trResult;
                return {};
            };

            QTC_CHECK_EXPECTED(callback(dest));
            return callback;
        }
        return {};
    }

    BaseClientInterface *createInterface(ProjectExplorer::Project *project)
    {
        if (m_transportType == TransportType::StdIO) {
            auto interface = new StdIOClientInterface;
            interface->setCommandLine(m_cmdLine);
            if (project)
                interface->setWorkingDirectory(project->projectDirectory());
            return interface;
        } else if (m_transportType == TransportType::LocalSocket) {
            if (m_serverName.isEmpty())
                return nullptr;

            auto interface = new LuaLocalSocketClientInterface(m_cmdLine, m_serverName);
            if (project)
                interface->setWorkingDirectory(project->projectDirectory());
            return interface;
        }
        return nullptr;
    }

signals:
    void optionsChanged();
};

LuaClientSettings::LuaClientSettings(const LuaClientSettings &other)
    : BaseSettings::BaseSettings(other)
    , m_wrapper(other.m_wrapper)
{
    if (auto w = m_wrapper.lock()) {
        QObject::connect(w.get(), &LuaClientWrapper::optionsChanged, &guard, [this] {
            if (auto w = m_wrapper.lock())
                m_initializationOptions = w->m_initializationOptions;
        });
    }
}

LuaClientSettings::LuaClientSettings(const std::weak_ptr<LuaClientWrapper> &wrapper)
    : m_wrapper(wrapper)
{
    if (auto w = m_wrapper.lock()) {
        m_name = w->m_name;
        m_settingsTypeId = w->m_settingsTypeId;
        m_languageFilter = w->m_languageFilter;
        m_initializationOptions = w->m_initializationOptions;
        m_startBehavior = w->m_startBehavior;
        m_showInSettings = w->m_showInSettings;
        QObject::connect(w.get(), &LuaClientWrapper::optionsChanged, &guard, [this] {
            if (auto w = m_wrapper.lock())
                m_initializationOptions = w->m_initializationOptions;
        });
    }
}

bool LuaClientSettings::applyFromSettingsWidget(QWidget *widget)
{
    BaseSettings::applyFromSettingsWidget(widget);

    if (auto w = m_wrapper.lock()) {
        w->m_name = m_name;
        if (!w->m_initOptionsCallback)
            w->m_initializationOptions = m_initializationOptions;
        w->m_languageFilter = m_languageFilter;
        w->m_startBehavior = m_startBehavior;
        w->applySettings();
    }

    return true;
}

Utils::Store LuaClientSettings::toMap() const
{
    auto store = BaseSettings::toMap();
    if (auto w = m_wrapper.lock())
        w->toMap(store);
    return store;
}

void LuaClientSettings::fromMap(const Utils::Store &map)
{
    BaseSettings::fromMap(map);
    if (auto w = m_wrapper.lock()) {
        w->m_name = m_name;
        if (!w->m_initOptionsCallback)
            w->m_initializationOptions = m_initializationOptions;
        w->m_languageFilter = m_languageFilter;
        w->m_startBehavior = m_startBehavior;
        w->fromMap(map);
    }
}

QWidget *LuaClientSettings::createSettingsWidget(QWidget *parent) const
{
    using namespace Layouting;

    if (auto w = m_wrapper.lock())
        return new BaseSettingsWidget(this, parent, w->settingsLayout());

    return new BaseSettingsWidget(this, parent);
}

Client *LuaClientSettings::createClient(BaseClientInterface *interface) const
{
    Client *client = new LuaClient(interface, m_settingsTypeId);
    return client;
}

BaseClientInterface *LuaClientSettings::createInterface(ProjectExplorer::Project *project) const
{
    if (auto w = m_wrapper.lock())
        return w->createInterface(project);

    return nullptr;
}

static void registerLuaApi()
{
    ::Lua::registerProvider("LSP", [](sol::state_view lua) -> sol::object {
        sol::table async = lua.script("return require('async')", "_process_").get<sol::table>();
        sol::function wrap = async["wrap"];

        sol::table result = lua.create_table();

        auto wrapperClass = result.new_usertype<LuaClientWrapper>(
            "Client",
            "on_instance_start",
            sol::property(
                [](const LuaClientWrapper *c) -> sol::function {
                    if (!c->m_onInstanceStart)
                        return sol::lua_nil;
                    return c->m_onInstanceStart.value();
                },
                [](LuaClientWrapper *c, const sol::function &f) { c->m_onInstanceStart = f; }),
            "registerMessage",
            &LuaClientWrapper::registerMessageCallback,
            "sendMessage",
            &LuaClientWrapper::sendMessage,
            "sendMessageForDocument",
            &LuaClientWrapper::sendMessageForDocument,
            "sendMessageWithIdForDocument_cb",
            &LuaClientWrapper::sendMessageWithIdForDocument_cb,
            "create",
            [](const sol::table &options) -> std::shared_ptr<LuaClientWrapper> {
                auto luaClientWrapper = std::make_shared<LuaClientWrapper>(options);
                auto clientSettings = new LuaClientSettings(luaClientWrapper);

                // The order is important!
                // First restore the settings ...
                const QList<Utils::Store> savedSettings
                    = LanguageClientSettings::storesBySettingsType(
                        luaClientWrapper->m_settingsTypeId);

                if (!savedSettings.isEmpty())
                    clientSettings->fromMap(savedSettings.first());

                // ... then register the settings.
                LanguageClientManager::registerClientSettings(clientSettings);
                luaClientWrapper->m_clientSettingsId = clientSettings->m_id;

                // and the client type.
                ClientType type;
                type.id = clientSettings->m_settingsTypeId;
                type.name = luaClientWrapper->m_name;
                type.userAddable = false;
                LanguageClientSettings::registerClientType(type);

                return luaClientWrapper;
            },
            "documentVersion",
            [](LuaClientWrapper *wrapper,
               const Utils::FilePath &path) -> std::tuple<bool, std::variant<int, QString>> {
                auto clients = wrapper->clientsForDocument(
                    TextEditor::TextDocument::textDocumentForFilePath(path));
                if (clients.empty())
                    return {false, "No client found."};

                return {true, clients.first()->documentVersion(path)};
            },

            "hostPathToServerUri",
            [](LuaClientWrapper *wrapper, const Utils::FilePath &path) -> std::tuple<bool, QString> {
                auto clients = wrapper->clientsForDocument(
                    TextEditor::TextDocument::textDocumentForFilePath(path));
                if (clients.empty())
                    return {false, "No client found."};

                return {true, clients.first()->hostPathToServerUri(path).toString()};
            });

        wrapperClass["sendMessageWithIdForDocument"]
            = wrap(wrapperClass["sendMessageWithIdForDocument_cb"].get<sol::function>())
                  .get<sol::function>();

        return result;
    });
}

} // namespace LanguageClient::Lua

#include "lualanguageclient.moc"
