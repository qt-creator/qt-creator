// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include <utils/stylehelper.h>

QT_BEGIN_NAMESPACE
class QFont;
class QLabel;
class QWidget;
QT_END_NAMESPACE

namespace Utils { class MarkdownBrowser; }

namespace AcpClient::Internal {

class ChatFontScale : public QObject
{
    Q_OBJECT

public:
    static ChatFontScale &instance();

    static qreal scale();
    static void setScale(qreal scale);
    static void zoomIn();
    static void zoomOut();
    static void resetZoom();

signals:
    void scaleChanged(qreal scale);

private:
    ChatFontScale();

    qreal m_scale;
};

void setupChatZoomActions(QObject *guard);
void setChatFont(QWidget *widget, qreal factor = 1.0);
void setChatFont(QWidget *widget, const QFont &baseFont);
void setChatTextFormat(QLabel *label, const Utils::StyleHelper::TextFormat &format);
void setChatSpacing(QWidget *widget);
qreal chatRadius(qreal radius);
void setupChatBrowser(Utils::MarkdownBrowser *browser);
void enableChatZoom(QWidget *widget);

} // namespace AcpClient::Internal
