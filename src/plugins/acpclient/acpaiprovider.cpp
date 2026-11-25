// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpaiprovider.h"

#include "acpchatcontroller.h"
#include "acpchattab.h"
#include "acpclienttr.h"
#include "acpsettings.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/async.h>
#include <utils/qtcsettings.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>

#include <algorithm>

using namespace Acp::V2;
using namespace Utils;

namespace AcpClient::Internal {

const char configOptionsKey[] = "AcpClient/AgentConfigOptions";

AcpPromptRequest acpPromptRequest(const AiRequest &request, const AiTargetSelection &selection)
{
    AcpPromptRequest result;
    result.workingDirectory = request.workingDirectory;
    if (result.workingDirectory.isEmpty()) {
        using namespace ProjectExplorer;
        if (const Project *project = ProjectManager::startupProject())
            result.workingDirectory = project->projectDirectory();
    }
    result.configValues = selection.parameters;
    result.prompt = request.prompt;
    for (const AiContextItem &item : request.context) {
        switch (item.kind) {
        case AiContextItem::File:
            result.files.append(item.filePath);
            if (item.range) {
                // The file goes along with its path, so the name of the excerpt only
                // needs to tell it apart from other context.
                const int first = item.range->begin.line;
                const int last = item.range->end.line;
                const QString lines = last > first ? QString("%1-%2").arg(first).arg(last)
                                                   : QString::number(first);
                const QString name = item.filePath.fileName() + ':' + lines;
                result.textContexts.append(
                    {name, AiAssistance::excerpt(item.filePath, *item.range), TextContextScope::None});
            }
            break;
        case AiContextItem::Text:
            result.textContexts.append({item.name, item.text, TextContextScope::None});
            break;
        case AiContextItem::Image:
            result.imageContexts.append({item.name, item.image});
            break;
        }
    }
    return result;
}

static AiTargetParameter::Category parameterCategory(const SessionConfigOption &option)
{
    if (!option.category().has_value())
        return AiTargetParameter::Other;
    switch (*option.category()) {
    case SessionConfigOptionCategory::model:
        return AiTargetParameter::Model;
    case SessionConfigOptionCategory::thought_level:
        return AiTargetParameter::Effort;
    case SessionConfigOptionCategory::mode:
        return AiTargetParameter::Mode;
    case SessionConfigOptionCategory::model_config:
        break;
    }
    return AiTargetParameter::Other;
}

QList<AiTargetParameter> aiTargetParameters(const QList<SessionConfigOption> &configOptions)
{
    QList<AiTargetParameter> result;
    for (const SessionConfigOption &option : configOptions) {
        const auto select = fromJson<SessionConfigSelect>(QJsonValue(toJson(option)));
        if (!select)
            continue;
        AiTargetParameter parameter;
        parameter.id = option.configId();
        parameter.displayName = option.name();
        parameter.category = parameterCategory(option);
        // Like the chat, offer only the first option of a category. A v1 agent
        // can report a mode option of its own besides its session modes.
        const auto sameCategory = [&parameter](const AiTargetParameter &other) {
            return other.category == parameter.category;
        };
        if (parameter.category != AiTargetParameter::Other
            && std::any_of(result.cbegin(), result.cend(), sameCategory)) {
            continue;
        }
        parameter.defaultValue = select->currentValue();
        const auto addChoice = [&parameter](const SessionConfigSelectOption &choice) {
            const QString description = choice.description().has_value()
                                            ? *choice.description()
                                            : QString();
            parameter.choices.append({choice.value(), choice.name(), description});
        };
        if (const auto flat = std::get_if<QList<SessionConfigSelectOption>>(&select->options())) {
            for (const SessionConfigSelectOption &choice : *flat)
                addChoice(choice);
        } else if (const auto groups
                   = std::get_if<QList<SessionConfigSelectGroup>>(&select->options())) {
            for (const SessionConfigSelectGroup &group : *groups) {
                for (const SessionConfigSelectOption &choice : group.options())
                    addChoice(choice);
            }
        }
        if (!parameter.choices.isEmpty())
            result.append(parameter);
    }
    return result;
}

static QMap<QString, QList<SessionConfigOption>> rememberedConfigOptions()
{
    QMap<QString, QList<SessionConfigOption>> result;
    const QVariantMap map = userSettings().value(configOptionsKey).toMap();
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        const QJsonArray array = QJsonDocument::fromJson(it.value().toString().toUtf8()).array();
        QList<SessionConfigOption> options;
        for (const QJsonValue &value : array) {
            if (const auto option = fromJson<SessionConfigOption>(value))
                options.append(*option);
        }
        result.insert(it.key(), options);
    }
    return result;
}

class AcpAiProvider;
static AcpAiProvider *theProvider = nullptr;

class AcpHeadlessRun : public QObject
{
public:
    AcpHeadlessRun(const AcpSettings::ServerInfo &server, const AcpPromptRequest &request,
                   AcpInspector *inspector, AiResponse *response)
        : QObject(response)
        , m_request(request)
        , m_response(response)
    {
        m_controller = new AcpChatController(this);
        m_controller->setInspector(inspector);

        connect(response, &AiResponse::cancelRequested,
                m_controller, &AcpChatController::cancelPrompt);
        connect(m_controller, &AcpChatController::sessionSelectionRequired, this, [this] {
            if (m_sessionRequested)
                return;
            m_sessionRequested = true;
            m_controller->createNewSession(m_request.workingDirectory);
        });
        connect(m_controller, &AcpChatController::authenticationRequired, this, [this] {
            fail(Tr::tr("The agent requires authentication. Sign in from the AI chat first."));
        });
        connect(m_controller, &AcpChatController::sessionCreated, this, [this] {
            m_configuring = true;
            m_controller->setConfigOptions(m_request.configValues, [this] {
                m_configuring = false;
                m_prompting = true;
                m_controller->sendPrompt(m_request.prompt, m_request.files, false,
                                         m_request.textContexts, m_request.imageContexts);
            });
        });
        connect(m_controller, &AcpChatController::sessionUpdate,
                this, [this](const QString &, const SessionUpdate &update) {
            if (update.kind() != QLatin1String("agent_message_chunk"))
                return;
            if (const auto chunk = update.get<ContentChunk>()) {
                if (const auto text = std::get_if<TextContent>(&chunk->content())) {
                    m_text += text->text();
                    m_response->reportText(text->text());
                }
            }
        });
        connect(m_controller, &AcpChatController::promptFinished,
                this, [this](const std::optional<QString> &stopReason) {
            if (!m_prompting)
                return;
            // Any other stop reason leaves the answer incomplete. The text that did
            // arrive was reported as it came.
            if (stopReason && *stopReason != QLatin1String("end_turn"))
                fail(Tr::tr("The agent stopped early: %1").arg(*stopReason));
            else if (!stopReason && m_text.isEmpty())
                fail(Tr::tr("The agent ended its turn without an answer."));
            else
                m_response->reportFinished(m_text);
        });
        connect(m_controller, &AcpChatController::errorOccurred,
                this, [this](const QString &error) {
            // A rejected option leaves the agent's own choice in place.
            if (!m_configuring)
                fail(error);
        });
        connect(m_controller, &AcpChatController::connectionStateChanged,
                this, [this](AcpClientObject::State state) {
            if (state == AcpClientObject::State::Disconnected)
                fail(Tr::tr("The agent disconnected."));
        });
        connect(m_controller, &AcpChatController::permissionRequested,
                this, [this](const QJsonValue &id, const RequestPermissionRequest &request) {
            for (const PermissionOption &option : request.options()) {
                if (option.kind() == PermissionOptionKind::reject_once
                    || option.kind() == PermissionOptionKind::reject_always) {
                    m_controller->sendPermissionResponse(id, option.optionId());
                    return;
                }
            }
            m_controller->sendPermissionCancelled(id);
        });
        connect(m_controller, &AcpChatController::elicitationRequested,
                m_controller, &AcpChatController::sendElicitationDeclined);

        m_controller->connectToServer(server);
    }

    ~AcpHeadlessRun() override
    {
        // The controller disconnects on destruction, after the response is gone.
        disconnect(m_controller, nullptr, this, nullptr);
    }

private:
    void fail(const QString &error)
    {
        m_response->reportFinished(ResultError(error));
    }

    AcpPromptRequest m_request;
    AiResponse *m_response = nullptr;
    AcpChatController *m_controller = nullptr;
    QString m_text;
    bool m_sessionRequested = false;
    bool m_configuring = false;
    bool m_prompting = false;
};

class AcpAiProvider final : public AiProvider
{
public:
    AcpAiProvider(AcpInspector *inspector,
                  const std::function<void(AcpChatTab *)> &showTab,
                  const std::function<void()> &showChat)
        : AiProvider("AcpClient.Agents", Tr::tr("ACP Agents"))
        , m_inspector(inspector)
        , m_showTab(showTab)
        , m_showChat(showChat)
    {
        theProvider = this;
        connect(&AcpSettings::instance(), &AcpSettings::serversChanged,
                this, &AcpAiProvider::updateTargets);
        updateTargets();
    }

    ~AcpAiProvider() final
    {
        theProvider = nullptr;
    }

    void updateTargets()
    {
        const QMap<QString, QList<SessionConfigOption>> configOptions = rememberedConfigOptions();
        QList<AiTarget> targets;
        for (const AcpSettings::ServerInfo &server : AcpSettings::servers()) {
            AiTarget target;
            target.id = server.id;
            target.displayName = server.name;
            target.description = Tr::tr("Opens a new AI chat with %1.").arg(server.name);
            target.icon = m_icons.value(server.iconUrl);
            target.supportsHeadless = true;
            target.parameters = aiTargetParameters(configOptions.value(server.id));
            targets.append(target);
            fetchIcon(server.iconUrl);
        }
        setTargets(targets);
    }

    void execute(const AiRequest &request, const AiTargetSelection &selection,
                 AiResponse *response) final
    {
        if (request.prompt.trimmed().isEmpty()) {
            response->reportFinished(ResultError(Tr::tr("The request has no prompt.")));
            return;
        }
        if (acpTermsPending()) {
            if (request.mode == AiRequest::Interactive)
                m_showChat();
            response->reportFinished(
                ResultError(Tr::tr("Accept the terms and conditions of the AI chat first.")));
            return;
        }

        const QList<AcpSettings::ServerInfo> servers = AcpSettings::servers();
        const auto server = std::find_if(servers.cbegin(), servers.cend(),
                                         [&selection](const AcpSettings::ServerInfo &info) {
            return info.id == selection.targetId;
        });
        if (server == servers.cend()) {
            response->reportFinished(ResultError(Tr::tr("The agent is not configured anymore.")));
            return;
        }

        AcpPromptRequest promptRequest = acpPromptRequest(request, selection);
        if (request.mode == AiRequest::Headless) {
            runHeadlessAcpRequest(*server, promptRequest, m_inspector, response);
            return;
        }

        QPointer<AiResponse> guard(response);
        promptRequest.onSent = [guard](const Result<> &result) {
            if (!guard)
                return;
            if (result)
                guard->reportFinished(QString());
            else
                guard->reportFinished(ResultError(result.error()));
        };
        auto tab = new AcpChatTab;
        tab->setInspector(m_inspector);
        m_showTab(tab);
        tab->startRequest(*server, promptRequest);
    }

private:
    void fetchIcon(const QString &url)
    {
        if (url.isEmpty() || m_icons.contains(url) || m_iconsRequested.contains(url))
            return;
        m_iconsRequested.insert(url);
        Utils::onResultReady(AcpSettings::iconForUrl(url), this, [this, url](const QIcon &icon) {
            m_icons.insert(url, icon);
            updateTargets();
        });
    }

    AcpInspector *m_inspector = nullptr;
    std::function<void(AcpChatTab *)> m_showTab;
    std::function<void()> m_showChat;
    QHash<QString, QIcon> m_icons;
    QSet<QString> m_iconsRequested;
};

/*!
    Runs \a request on \a server without showing it, and reports the answer
    of the agent to \a response. The agent is refused every permission it
    asks for, and cannot ask the user for input.
*/
void runHeadlessAcpRequest(const AcpSettings::ServerInfo &server, const AcpPromptRequest &request,
                           AcpInspector *inspector, AiResponse *response)
{
    new AcpHeadlessRun(server, request, inspector, response);
}

void rememberAgentConfigOptions(const QString &serverId,
                                const QList<SessionConfigOption> &configOptions)
{
    if (serverId.isEmpty())
        return;
    QJsonArray array;
    for (const SessionConfigOption &option : configOptions)
        array.append(toJson(option));
    const QString json = QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
    QVariantMap map = userSettings().value(configOptionsKey).toMap();
    if (map.value(serverId).toString() == json)
        return;
    map.insert(serverId, json);
    userSettings().setValue(configOptionsKey, map);
    if (theProvider)
        theProvider->updateTargets();
}

AiProvider *createAcpAiProvider(AcpInspector *inspector,
                                const std::function<void(AcpChatTab *)> &showTab,
                                const std::function<void()> &showChat)
{
    return new AcpAiProvider(inspector, showTab, showChat);
}

} // namespace AcpClient::Internal
