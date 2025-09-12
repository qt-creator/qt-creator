// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "manifest.h"

#include <studioquickwidget.h>

#include <utils/uniqueobjectptr.h>

#include <QFrame>
#include <QNetworkAccessManager>
#include <QPointer>

#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
class QNetworkReply;
class QTextEdit;
class QToolButton;
QT_END_NAMESPACE

namespace QmlDesigner {

class AiResponse;
class AiAssistantWidget : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(bool isGenerating MEMBER m_isGenerating NOTIFY isGeneratingChanged FINAL)
    Q_PROPERTY(QString attachedImageSource READ attachedImageSource WRITE setAttachedImageSource
                   NOTIFY attachedImageSourceChanged FINAL)

public:
    AiAssistantWidget();
    ~AiAssistantWidget() = default;

    QString attachedImageSource() const;
    void setAttachedImageSource(const QString &source);

    void clearAttachedImage();
    void initManifest();

    QSize sizeHint() const override;

    Q_INVOKABLE QStringList getImageAssetsPaths() const;
    Q_INVOKABLE void handleMessage(const QString &prompt);
    Q_INVOKABLE QString getPreviousCommand();
    Q_INVOKABLE QString getNextCommand();
    Q_INVOKABLE QUrl fullImageUrl(const QString &path) const;

signals:
    void isGeneratingChanged();
    void attachedImageSourceChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private: // functions
    void reloadQmlSource();
    void setIsGenerating(bool val);
    void handleAiResponse(const AiResponse &response);

private: // variables
    std::unique_ptr<QNetworkAccessManager> m_manager;
    Utils::UniqueObjectPtr<StudioQuickWidget> m_quickWidget;

    QStringList m_inputHistory;
    QString m_attachedImageSource;
    Manifest m_manifest;
    int m_historyIndex = -1;
    bool m_isGenerating = false;
};

} // namespace QmlDesigner
