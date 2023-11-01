// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "helpviewer.h"
#include "openpagesmanager.h"

#include <QTextBrowser>

namespace Help {
namespace Internal {

class TextBrowserHelpWidget;

class TextBrowserHelpViewer : public HelpViewer
{
    Q_OBJECT

public:
    explicit TextBrowserHelpViewer(QWidget *parent = nullptr);
    ~TextBrowserHelpViewer() override;

    void setViewerFont(const QFont &font) override;
    void setScale(qreal scale) override;
    void setAntialias(bool on) final;

    QString title() const override;

    QUrl source() const override;
    void setSource(const QUrl &url) override;

    void setHtml(const QString &html) override;

    QString selectedText() const override;
    bool isForwardAvailable() const override;
    bool isBackwardAvailable() const override;
    void addBackHistoryItems(QMenu *backMenu) override;
    void addForwardHistoryItems(QMenu *forwardMenu) override;

    bool findText(const QString &text, Utils::FindFlags flags,
                  bool incremental, bool fromSearch, bool *wrapped = nullptr) override;

    void copy() override;
    void stop() override;
    void forward() override;
    void backward() override;
    void print(QPrinter *printer) override;

private:
    void setFontAndScale(const QFont &font, qreal scale, bool antialias);

    TextBrowserHelpWidget *m_textBrowser;
};

class TextBrowserHelpWidget : public QTextBrowser
{
    Q_OBJECT

public:
    TextBrowserHelpWidget(TextBrowserHelpViewer *parent);

    QVariant loadResource(int type, const QUrl &name) override;

    void withFixedTopPosition(const std::function<void()> &action);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void wheelEvent(QWheelEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private:
    QString linkAt(const QPoint &pos);
    void scrollToTextPosition(int position);

    TextBrowserHelpViewer *m_parent;
    friend class Help::Internal::TextBrowserHelpViewer;
};

}   // namespace Internal
}   // namespace Help
