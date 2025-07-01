// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

public:
    AiAssistantWidget();
    ~AiAssistantWidget() = default;

    QList<QToolButton *> createToolBarWidgets();

    static QString qmlSourcesPath();
    void clearSearchFilter();

    QSize sizeHint() const override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void handleMessage();

    QPointer<QTextEdit> m_textInput;
    QPointer<QLabel> m_imageLabel;
    QStringList m_imagePaths;

    std::unique_ptr<QNetworkAccessManager> m_manager;
    QPointer<QNetworkReply> m_reply;

    QStringList m_inputHistory;
    int m_historyIndex = -1;
};

} // namespace QmlDesigner
