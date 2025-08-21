// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

class AiAssistantWidget : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(bool isGenerating MEMBER m_isGenerating NOTIFY isGeneratingChanged FINAL)

public:
    AiAssistantWidget();
    ~AiAssistantWidget() = default;

    void clearAttachedImage();

    QSize sizeHint() const override;

    Q_INVOKABLE QStringList getImageAssetsPaths() const;
    Q_INVOKABLE void handleMessage(const QString &prompt);
    Q_INVOKABLE QString getPreviousCommand();
    Q_INVOKABLE QString getNextCommand();
    Q_INVOKABLE QUrl fullImageUrl(const QString &path) const;

signals:
    void isGeneratingChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private: // functions
    void reloadQmlSource();
    void setIsGenerating(bool val);
    QString attachedImageSource() const;

private: // variables
    std::unique_ptr<QNetworkAccessManager> m_manager;
    QPointer<QNetworkReply> m_reply;

    Utils::UniqueObjectPtr<StudioQuickWidget> m_quickWidget;

    QStringList m_inputHistory;
    int m_historyIndex = -1;
    bool m_isGenerating = false;
};

} // namespace QmlDesigner
