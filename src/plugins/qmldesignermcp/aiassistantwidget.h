// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "aitransaction.h"
#include "chathistorymodel.h"
#include "manifest.h"

#include <utils/uniqueobjectptr.h>

#include <QAbstractItemModel>
#include <QFrame>
#include <QPointer>

#include <memory>

class StudioQuickWidget;

namespace QmlDesigner {

class AgenticRequestManager;
class AiAssistantView;
class AiModelsModel;
class AiResponse;
class ConversationManager;
class McpHost;
class ToolRegistry;

class AiAssistantWidget : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(bool termsAccepted MEMBER m_termsAccepted NOTIFY termsAcceptedChanged FINAL)
    Q_PROPERTY(bool isGenerating MEMBER m_isGenerating NOTIFY isGeneratingChanged FINAL)
    Q_PROPERTY(bool hasValidModel MEMBER m_hasValidModel NOTIFY hasValidModelChanged FINAL)
    Q_PROPERTY(QString attachedImageSource READ attachedImageSource WRITE setAttachedImageSource
                   NOTIFY attachedImageSourceChanged FINAL)
    Q_PROPERTY(QAbstractItemModel *modelsModel READ modelsModel CONSTANT)
    Q_PROPERTY(QAbstractListModel *chatHistory READ chatHistory CONSTANT)

public:
    AiAssistantWidget(AiAssistantView *view);
    ~AiAssistantWidget();

    QString attachedImageSource() const;
    void setAttachedImageSource(const QString &source);

    QAbstractItemModel *modelsModel() const;
    QAbstractListModel *chatHistory() const;

    void clear();
    void initManifest();
    void updateModelConfig();
    void removeMissingAttachedImage();
    void setProjectPath(const QString &projectPath);

    QSize sizeHint() const override;

    Q_INVOKABLE QStringList getImageAssetsPaths() const;
    Q_INVOKABLE void handleMessage(const QString &prompt);
    Q_INVOKABLE QUrl fullImageUrl(const QString &path) const;
    Q_INVOKABLE void retryLastPrompt();
    Q_INVOKABLE void applyLastGeneratedQml();
    Q_INVOKABLE void undoLastChange();
    Q_INVOKABLE void sendThumbFeedback(bool up);
    Q_INVOKABLE void openModelSettings();
    Q_INVOKABLE void openTermsDialog();
    Q_INVOKABLE void clearChat();

signals:
    void termsAcceptedChanged();
    void isGeneratingChanged();
    void hasValidModelChanged();
    void attachedImageSourceChanged();

    // from C++ to Qml
    void notifyAIResponseSuccess();
    void notifyAIResponseInvalidQml();
    void notifyAIResponseError(const QString &errMessage = {});
    void removeFeedbackPopup();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private: // functions
    void reloadQmlSource();
    void setIsGenerating(bool val);
    void setHasValidModel(bool val);
    void handleAiResponse(const AiResponse &response);
    void initializeMcp();
    void connectRequestManager();
    void connectMcpResourceSignals();
    void fetchProjectStructure();

private: // variables
    QShortcut *m_qmlSourceUpdateShortcut = nullptr;

    Utils::UniqueObjectPtr<StudioQuickWidget> m_quickWidget;
    Utils::UniqueObjectPtr<AiModelsModel> m_modelsModel;

    Utils::UniqueObjectPtr<ChatHistoryModel> m_chatHistory;
    Utils::UniqueObjectPtr<McpHost> m_mcpHost;
    Utils::UniqueObjectPtr<ToolRegistry> m_toolRegistry;
    std::unique_ptr<ConversationManager> m_conversation;
    Utils::UniqueObjectPtr<AgenticRequestManager> m_requestManager;

    QPointer<AiAssistantView> m_view;
    AiTransaction m_lastTransaction; // TODO: update to work with MCP
    QString m_attachedImageSource;
    QString m_projectPath;
    QString m_projectStructure;
    Manifest m_manifest;
    bool m_pendingStructureRefresh = false;
    bool m_termsAccepted = false;
    bool m_isGenerating = false;
    bool m_hasValidModel = false;
};

} // namespace QmlDesigner
