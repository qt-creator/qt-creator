// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmllsclient.h"

#include "qmljseditorconstants.h"
#include "qmljseditortr.h"
#include "qmljseditorsettings.h"
#include "qmljsquickfix.h"

#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientquickfix.h>

#include <projectexplorer/buildmanager.h>

#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/iassistprovider.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/mimeconstants.h>

#include <QLoggingCategory>
#include <QMetaEnum>
#include <optional>

using namespace LanguageClient;
using namespace Utils;

namespace {
Q_LOGGING_CATEGORY(qmllsLog, "qtc.qmlls.client", QtWarningMsg);
}

namespace QmlJSEditor {

static QHash<FilePath, QmllsClient *> &qmllsClients()
{
    static QHash<FilePath, QmllsClient *> clients;
    return clients;
}

QmllsClient *QmllsClient::clientForQmlls(const FilePath &qmlls)
{
    QTC_ASSERT(!qmlls.isEmpty(), return nullptr);
    if (auto client = qmllsClients()[qmlls]) {
        switch (client->state()) {
        case Client::State::Uninitialized:
        case Client::State::InitializeRequested:
        case Client::State::Initialized:
            return client;
        case Client::State::FailedToInitialize:
        case Client::State::ShutdownRequested:
        case Client::State::Shutdown:
        case Client::State::Error:
            qCDebug(qmllsLog) << "client was stopping or failed, restarting";
            break;
        }
    }
    auto interface = new StdIOClientInterface;
    interface->setCommandLine(CommandLine(qmlls));
    auto client = new QmllsClient(interface);
    client->setName(Tr::tr("Qmlls (%1)").arg(qmlls.toUserOutput()));
    client->setActivateDocumentAutomatically(true);
    LanguageFilter filter;
    using namespace Utils::Constants;
    filter.mimeTypes = {QML_MIMETYPE, QMLUI_MIMETYPE, QBS_MIMETYPE, QMLPROJECT_MIMETYPE,
                        QMLTYPES_MIMETYPE, JS_MIMETYPE, JSON_MIMETYPE};
    client->setSupportedLanguage(filter);
    client->start();
    qmllsClients()[qmlls] = client;
    return client;
}

QMap<QString, int> QmllsClient::semanticTokenTypesMap()
{
    QMap<QString, int> result;
    QMetaEnum metaEnum = QMetaEnum::fromType<QmllsClient::QmlSemanticTokens>();
    for (auto i = 0; i < metaEnum.keyCount(); ++i) {
        auto &&enumName = QString::fromUtf8(metaEnum.key(i));
        enumName.front() = enumName.front().toLower();
        result.insert(std::move(enumName), metaEnum.value(i));
    }

    return result;
}

void QmllsClient::updateQmllsSemanticHighlightingCapability()
{
    const QString methodName = QStringLiteral("textDocument/semanticTokens");
    if (!QmlJSEditor::Internal::settings().enableQmllsSemanticHighlighting()) {
        LanguageServerProtocol::Unregistration unregister;
        unregister.setMethod(methodName);
        unregister.setId({});
        this->unregisterCapabilities({unregister});
    } else {
        const LanguageServerProtocol::ServerCapabilities &caps = this->capabilities();
        const std::optional<LanguageServerProtocol::SemanticTokensOptions> &options
            = caps.semanticTokensProvider();
        if (options) {
            LanguageServerProtocol::Registration registeration;
            registeration.setMethod(methodName);
            registeration.setId({});
            registeration.setRegisterOptions(QJsonObject{*options});
            this->registerCapabilities({registeration});
        } else {
            qCWarning(qmllsLog) << "qmlls does not support semantic highlighting";
        }
    }
}

class QmllsQuickFixAssistProcessor : public LanguageClientQuickFixAssistProcessor
{
public:
    using LanguageClientQuickFixAssistProcessor::LanguageClientQuickFixAssistProcessor;
private:
    TextEditor::GenericProposal *perform() override
    {
        // Step 1: Collect qmlls code actions asynchronously
        LanguageClientQuickFixAssistProcessor::perform();

        // Step 2: Collect built-in quickfixes synchronously
        m_builtinOps = findQmlJSQuickFixes(interface());

        return nullptr;
    }

    TextEditor::GenericProposal *handleCodeActionResult(const LanguageServerProtocol::CodeActionResult &result) override
    {
        return TextEditor::GenericProposal::createProposal(
            interface(), resultToOperations(result) + m_builtinOps);
    }

    QuickFixOperations m_builtinOps;
};

class QmllsQuickFixAssistProvider : public LanguageClientQuickFixProvider
{
public:
    using LanguageClientQuickFixProvider::LanguageClientQuickFixProvider;
    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const override
    {
        return new QmllsQuickFixAssistProcessor(client());
    }
};

QmllsClient::QmllsClient(StdIOClientInterface *interface)
    : Client(interface)
{
    setSnippetsGroup(QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID);

    connect(
        ProjectExplorer::BuildManager::instance(),
        &ProjectExplorer::BuildManager::buildQueueFinished,
        this,
        [this]() { LanguageClientManager::restartClient(this); });
    QJsonObject initializationOptions {
        {"qtCreatorHighlighting", true}
    };
    setInitializationOptions(initializationOptions);
    semanticTokenSupport()->setTokenTypesMap(QmllsClient::semanticTokenTypesMap());
    semanticTokenSupport()->setTextStyleForTokenType(
        [](int tokenType) -> std::optional<TextEditor::TextStyle> {
            using namespace TextEditor;
            switch (tokenType) {
            // customized lsp token types
            case QmlSemanticTokens::Namespace:
                return C_NAMESPACE;
            case QmlSemanticTokens::Type:
                return C_QML_TYPE_ID;
            case QmlSemanticTokens::Enum:
                return C_ENUMERATION;
            case QmlSemanticTokens::Parameter:
                return C_PARAMETER;
            case QmlSemanticTokens::Variable:
                return C_JS_SCOPE_VAR;
            case QmlSemanticTokens::Property:
                return C_BINDING;
            case QmlSemanticTokens::EnumMember:
                return C_FIELD;
            case QmlSemanticTokens::Method:
                return C_FUNCTION;
            case QmlSemanticTokens::Keyword:
                return C_KEYWORD;
            case QmlSemanticTokens::Comment:
                return C_COMMENT;
            case QmlSemanticTokens::String:
                return C_STRING;
            case QmlSemanticTokens::Number:
                return C_NUMBER;
            case QmlSemanticTokens::Regexp:
                return C_STRING;
            case QmlSemanticTokens::Operator:
                return C_OPERATOR;
            case QmlSemanticTokens::QmlLocalId:
                return C_QML_LOCAL_ID;
            case QmlSemanticTokens::QmlExternalId:
                return C_QML_EXTERNAL_ID;
            case QmlSemanticTokens::QmlRootObjectProperty:
                return C_QML_ROOT_OBJECT_PROPERTY;
            case QmlSemanticTokens::QmlScopeObjectProperty:
                return C_QML_SCOPE_OBJECT_PROPERTY;
            case QmlSemanticTokens::QmlExternalObjectProperty:
                return C_QML_EXTERNAL_OBJECT_PROPERTY;
            case QmlSemanticTokens::JsScopeVar:
                return C_JS_SCOPE_VAR;
            case QmlSemanticTokens::JsImportVar:
                return C_JS_IMPORT_VAR;
            case QmlSemanticTokens::JsGlobalVar:
                return C_JS_GLOBAL_VAR;
            case QmlSemanticTokens::QmlStateName:
                return C_QML_STATE_NAME;
            default:
                break;
            }
            return std::nullopt;
        });
    setQuickFixAssistProvider(new QmllsQuickFixAssistProvider(this));
}

QmllsClient::~QmllsClient()
{
    qmllsClients().remove(qmllsClients().key(this));
}

void QmllsClient::startImpl()
{
    updateQmllsSemanticHighlightingCapability();
    Client::startImpl();
}

} // namespace QmlJSEditor
