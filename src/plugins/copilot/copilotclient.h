// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "copilotrequests.h"

#include <languageclient/client.h>

#include <utils/filepath.h>

#include <QHash>
#include <QTemporaryDir>

namespace Copilot::Internal {

class CopilotClient final : public LanguageClient::Client
{
public:
    CopilotClient(const Utils::FilePath &nodePath, const Utils::FilePath &distPath);
    ~CopilotClient() override;

    void openDocument(TextEditor::TextDocument *document) override;

    void scheduleRequest(TextEditor::TextEditorWidget *editor);
    void requestCompletions(TextEditor::TextEditorWidget *editor);
    void handleCompletions(
        const Utils::Result<Completions> &result,
        const LanguageServerProtocol::Position &requestPosition,
        TextEditor::TextEditorWidget *editor);
    void cancelRunningRequest(TextEditor::TextEditorWidget *editor);

    using StatusHandler = std::function<void(const Utils::Result<Status> &)>;
    void requestCheckStatus(bool localChecksOnly, const StatusHandler &callback);
    void requestSignOut(const StatusHandler &callback);
    void requestSignInInitiate(
        const std::function<void(const Utils::Result<SignInInitiateResult> &)> &callback);
    void requestSignInConfirm(const QString &userCode, const StatusHandler &callback);

    bool canOpenProject(ProjectExplorer::Project *project) override;

private:
    QHash<TextEditor::TextEditorWidget *, LanguageServerProtocol::MessageId> m_runningRequests;
    struct ScheduleData
    {
        int cursorPosition = -1;
        QTimer *timer = nullptr;
    };
    QHash<TextEditor::TextEditorWidget *, ScheduleData> m_scheduledRequests;
};

} // namespace Copilot::Internal
