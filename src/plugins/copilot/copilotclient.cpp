// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "copilotclient.h"
#include "copilotsettings.h"

#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientsettings.h>
#include <languageserverprotocol/lsputils.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/projectmanager.h>

#include <texteditor/texteditor.h>
#include <texteditor/textsuggestion.h>

#include <utils/filepath.h>

#include <QGuiApplication>
#include <QInputDialog>
#include <QLoggingCategory>
#include <QTimer>
#include <QToolButton>

using namespace LanguageServerProtocol;
using namespace TextEditor;
using namespace Utils;
using namespace ProjectExplorer;
using namespace Core;

static Q_LOGGING_CATEGORY(copilotClientLog, "qtc.copilot.client", QtWarningMsg)

namespace Copilot::Internal {

static LanguageClient::BaseClientInterface *clientInterface(const FilePath &nodePath,
                                                            const FilePath &distPath)
{
    CommandLine cmd{nodePath, {distPath.toFSPathString(), "--stdio"}};

    const auto interface = new LanguageClient::StdIOClientInterface;
    interface->setCommandLine(cmd);
    return interface;
}

CopilotClient::CopilotClient(const FilePath &nodePath, const FilePath &distPath)
    : LanguageClient::Client(clientInterface(nodePath, distPath))
{
    setName("Copilot");
    LanguageClient::LanguageFilter langFilter;

    langFilter.filePattern = {"*"};

    setSupportedLanguage(langFilter);
    setActivatable(false);
    setInitializationOptions({
        {"editorInfo",
         QJsonObject{{"name", qApp->applicationName()}, {"version", qApp->applicationVersion()}}},
        {"editorPluginInfo",
         QJsonObject{{"name", "Copilot"}, {"version", qApp->applicationVersion()}}},
    });

    registerCustomMethod("LogMessage", [](const QJsonObject &message) {
        qCDebug(copilotClientLog) << message
                                         .value("params")
                                         .toObject()
                                         .value("message")
                                         .toString();
        return true;
    });

    const QString p = settings().proxy();

    QJsonObject settingsRoot{
        {"github-enterprise", QJsonObject{{"uri", settings().githubEnterpriseUrl()}}},
        {"http",
         QJsonObject{{"proxyStrictSSL", settings().proxyRejectUnauthorized()}, {"proxy", p}}}};

    updateConfiguration(settingsRoot);

    start();

    auto openDoc = [this](IDocument *document) {
        if (auto *textDocument = qobject_cast<TextDocument *>(document))
            openDocument(textDocument);
    };

    connect(EditorManager::instance(), &EditorManager::documentOpened, this, openDoc);
    connect(EditorManager::instance(),
            &EditorManager::documentClosed,
            this,
            [this](IDocument *document) {
                if (auto textDocument = qobject_cast<TextDocument *>(document))
                    closeDocument(textDocument);
            });

    for (IDocument *doc : DocumentModel::openedDocuments())
        openDoc(doc);
}

CopilotClient::~CopilotClient() = default;

void CopilotClient::openDocument(TextDocument *document)
{
    auto project = ProjectManager::projectForFile(document->filePath());
    if (!isCopilotEnabled(project))
        return;

    Client::openDocument(document);
    connect(document,
            &TextDocument::contentsChangedWithPosition,
            this,
            [this, document](int position, int charsRemoved, int charsAdded) {
                Q_UNUSED(charsRemoved)
                if (!settings().autoComplete())
                    return;

                auto project = ProjectManager::projectForFile(document->filePath());
                if (!isCopilotEnabled(project))
                    return;

                auto textEditor = BaseTextEditor::currentTextEditor();
                if (!textEditor || textEditor->document() != document)
                    return;
                TextEditorWidget *widget = textEditor->editorWidget();
                if (widget->isReadOnly() || widget->multiTextCursor().hasMultipleCursors())
                    return;
                const int cursorPosition = widget->textCursor().position();
                if (cursorPosition < position || cursorPosition > position + charsAdded)
                    return;
                scheduleRequest(widget);
            });
}

void CopilotClient::scheduleRequest(TextEditorWidget *editor)
{
    cancelRunningRequest(editor);

    auto it = m_scheduledRequests.find(editor);
    if (it == m_scheduledRequests.end()) {
        auto timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this, editor]() {
            if (m_scheduledRequests[editor].cursorPosition == editor->textCursor().position())
                requestCompletions(editor);
        });
        connect(editor, &TextEditorWidget::destroyed, this, [this, editor]() {
            delete m_scheduledRequests.take(editor).timer;
            cancelRunningRequest(editor);
        });
        connect(editor, &TextEditorWidget::cursorPositionChanged, this, [this, editor] {
            cancelRunningRequest(editor);
        });
        it = m_scheduledRequests.insert(editor, {editor->textCursor().position(), timer});
    } else {
        it->cursorPosition = editor->textCursor().position();
    }
    it->timer->start(500);
}

void CopilotClient::requestCompletions(TextEditorWidget *editor)
{
    auto project = ProjectManager::projectForFile(editor->textDocument()->filePath());

    if (!isCopilotEnabled(project))
        return;

    MultiTextCursor cursor = editor->multiTextCursor();
    if (cursor.hasMultipleCursors() || cursor.hasSelection() || editor->suggestionVisible())
        return;

    const FilePath filePath = editor->textDocument()->filePath();
    const Position position = positionOf(cursor.mainCursor());
    const GetCompletionParams params{uriFor(filePath), documentVersion(filePath), position};
    m_runningRequests[editor] = sendRequest<GetCompletionRequest>(
        params,
        [this, editor = QPointer<TextEditorWidget>(editor), position](
            const Utils::Result<Completions> &result) {
            QTC_ASSERT(editor, return);
            handleCompletions(result, position, editor);
        });
}

void CopilotClient::handleCompletions(const Utils::Result<Completions> &result,
                                      const Position &requestPosition,
                                      TextEditorWidget *editor)
{
    m_runningRequests.remove(editor);
    if (!result) {
        log(QtCriticalMsg, result.error());
        return;
    }

    const MultiTextCursor cursors = editor->multiTextCursor();
    if (cursors.hasMultipleCursors())
        return;

    if (cursors.hasSelection()
        || cursors.mainCursor().position() != positionInDocument(requestPosition,
                                                                 editor->document())) {
        return;
    }

    auto isValidCompletion = [](const Completion &completion) {
        return !completion.text.trimmed().isEmpty();
    };
    QList<Completion> completions = Utils::filtered(result->completions, isValidCompletion);

    // remove trailing whitespaces from the end of the completions
    for (Completion &completion : completions) {
        if (completion.range.start().line() != completion.range.end().line())
            continue; // do not remove trailing whitespaces for multi-line replacements

        const QString completionText = completion.text;
        const int end = int(completionText.size()) - 1; // empty strings have been removed above
        int delta = 0;
        while (delta <= end && completionText[end - delta].isSpace())
            ++delta;

        if (delta > 0)
            completion.text = completionText.chopped(delta);
    }
    if (completions.isEmpty())
        return;
    auto suggestions = Utils::transform(completions, [](const Completion &c) {
        auto toTextPos = [](const Position &pos) {
            return Text::Position{pos.line() + 1, pos.character()};
        };

        Text::Range range{toTextPos(c.range.start()), toTextPos(c.range.end())};
        return TextSuggestion::Data{range, toTextPos(c.position), c.text};
    });
    editor->insertSuggestion(
        std::make_unique<TextEditor::CyclicSuggestion>(suggestions, editor->document()));
}

void CopilotClient::cancelRunningRequest(TextEditor::TextEditorWidget *editor)
{
    const auto it = m_runningRequests.constFind(editor);
    if (it == m_runningRequests.constEnd())
        return;
    cancelRequest(*it);
    m_runningRequests.erase(it);
}

void CopilotClient::requestCheckStatus(bool localChecksOnly, const StatusHandler &callback)
{
    sendRequest<CheckStatusRequest>(CheckStatusParams{localChecksOnly}, callback);
}

void CopilotClient::requestSignOut(const StatusHandler &callback)
{
    sendRequest<SignOutRequest>({}, callback);
}

void CopilotClient::requestSignInInitiate(
    const std::function<void(const Utils::Result<SignInInitiateResult> &)> &callback)
{
    sendRequest<SignInInitiateRequest>({}, callback);
}

void CopilotClient::requestSignInConfirm(const QString &userCode, const StatusHandler &callback)
{
    sendRequest<SignInConfirmRequest>(SignInConfirmParams{userCode}, callback);
}

bool CopilotClient::canOpenProject(Project *project)
{
    return isCopilotEnabled(project);
}

} // namespace Copilot::Internal
