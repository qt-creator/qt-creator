// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/find/textfindconstants.h>

#include <utils/filesearch.h>

#include <QFont>
#include <QMenu>
#include <QPrinter>
#include <QString>
#include <QUrl>
#include <QWidget>

namespace Help {
namespace Internal {

class HelpViewer : public QWidget
{
    Q_OBJECT

public:
    enum class Action {
        NewPage = 0x01,
        ExternalWindow = 0x02
    };
    Q_DECLARE_FLAGS(Actions, Action)

    explicit HelpViewer(QWidget *parent = nullptr);
    ~HelpViewer() override;

    virtual void setViewerFont(const QFont &font) = 0;
    virtual void setAntialias(bool on);

    virtual void setScale(qreal scale) = 0;

    void setFontZoom(int percentage);
    void setScrollWheelZoomingEnabled(bool enabled);
    bool isScrollWheelZoomingEnabled() const;

    virtual QString title() const = 0;

    virtual QUrl source() const = 0;
    virtual void setSource(const QUrl &url) = 0;

    virtual void setHtml(const QString &html) = 0;

    virtual QString selectedText() const = 0;
    virtual bool isForwardAvailable() const = 0;
    virtual bool isBackwardAvailable() const = 0;
    virtual void addBackHistoryItems(QMenu *backMenu) = 0;
    virtual void addForwardHistoryItems(QMenu *forwardMenu) = 0;
    void setActionVisible(Action action, bool visible);
    bool isActionVisible(Action action);

    virtual bool findText(const QString &text, Utils::FindFlags flags,
        bool incremental, bool fromSearch, bool *wrapped = nullptr) = 0;

    bool handleForwardBackwardMouseButtons(QMouseEvent *e);

    static bool isLocalUrl(const QUrl &url);
    static bool canOpenPage(const QString &url);
    static QString mimeFromUrl(const QUrl &url);
    static bool launchWithExternalApp(const QUrl &url);

    void home();

    void scaleUp();
    void scaleDown();
    void resetScale();
    virtual void copy() = 0;
    virtual void stop() = 0;
    virtual void forward() = 0;
    virtual void backward() = 0;
    virtual void print(QPrinter *printer) = 0;

signals:
    void sourceChanged(const QUrl &);
    void titleChanged();
    void printRequested();
    void forwardAvailable(bool);
    void backwardAvailable(bool);
    void loadFinished();
    void newPageRequested(const QUrl &url);
    void externalPageRequested(const QUrl &url);

protected:
    void wheelEvent(QWheelEvent *event) override;

    void slotLoadStarted();
    void slotLoadFinished();

    void restoreOverrideCursor();

    Actions m_visibleActions;
    bool m_scrollWheelZoomingEnabled = true;
    int m_loadOverrideStack = 0;
private:
    void incrementZoom(int steps);
    void applyZoom(int percentage);
};

}   // namespace Internal
}   // namespace Help
