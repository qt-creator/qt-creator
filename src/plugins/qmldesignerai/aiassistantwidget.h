// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "aiproviderconfig.h"
#include "manifest.h"

#include <utils/uniqueobjectptr.h>

#include <QFrame>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class QTextEdit;
class QToolButton;
QT_END_NAMESPACE

class StudioQuickWidget;

namespace QmlDesigner {

class AiAssistantView;
class AiResponse;

class AiAssistantWidget : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(bool isGenerating MEMBER m_isGenerating NOTIFY isGeneratingChanged FINAL)
    Q_PROPERTY(bool hasValidModel MEMBER m_hasValidModel NOTIFY hasValidModelChanged FINAL)
    Q_PROPERTY(QString attachedImageSource READ attachedImageSource WRITE setAttachedImageSource
                   NOTIFY attachedImageSourceChanged FINAL)

public:
    AiAssistantWidget(AiAssistantView *view);
    ~AiAssistantWidget();

    QString attachedImageSource() const;
    void setAttachedImageSource(const QString &source);

    void clear();
    void initManifest();
    void updateModelConfig();
    void removeMissingAttachedImage();

    QSize sizeHint() const override;

    Q_INVOKABLE QStringList getImageAssetsPaths() const;
    Q_INVOKABLE void handleMessage(const QString &prompt);
    Q_INVOKABLE QString getPreviousCommand();
    Q_INVOKABLE QString getNextCommand();
    Q_INVOKABLE QUrl fullImageUrl(const QString &path) const;
    Q_INVOKABLE void retryLastPrompt();
    Q_INVOKABLE void applyLastGeneratedQml();
    Q_INVOKABLE void sendThumbFeedback(bool up);
    Q_INVOKABLE void openModelSettings();

signals:
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
    bool isValidQmlCode(const QString &qmlCode) const;

private: // variables
    Utils::UniqueObjectPtr<QNetworkAccessManager> m_manager;
    Utils::UniqueObjectPtr<StudioQuickWidget> m_quickWidget;

    QPointer<AiAssistantView> m_view;
    QStringList m_inputHistory;
    QString m_lastGeneratedQml;
    QString m_attachedImageSource;
    Manifest m_manifest;
    AiModelInfo m_modelInfo;
    int m_historyIndex = -1;
    bool m_isGenerating = false;
    bool m_hasValidModel = false;
};

} // namespace QmlDesigner
