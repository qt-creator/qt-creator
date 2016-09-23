/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "webenginehelpviewer.h"

#include "helpconstants.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"

#include <utils/qtcassert.h>

#include <QBuffer>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QVBoxLayout>
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
#include <QWebEngineContextMenuData>
#endif
#include <QWebEngineHistory>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineUrlRequestJob>

using namespace Help;
using namespace Help::Internal;

HelpUrlSchemeHandler::HelpUrlSchemeHandler(QObject *parent) :
    QWebEngineUrlSchemeHandler(parent)
{
}

void HelpUrlSchemeHandler::requestStarted(QWebEngineUrlRequestJob *job)
{
    const QUrl url = job->requestUrl();
    if (!HelpViewer::isLocalUrl(url)) {
        job->fail(QWebEngineUrlRequestJob::RequestDenied);
        return;
    }
    LocalHelpManager::HelpData data = LocalHelpManager::helpData(url);

    auto buffer = new QBuffer(job);
    buffer->setData(data.data);
    job->reply(data.mimeType.toUtf8(), buffer);
}

static HelpUrlSchemeHandler *helpUrlSchemeHandler()
{
    static HelpUrlSchemeHandler *schemeHandler = nullptr;
    if (!schemeHandler)
        schemeHandler = new HelpUrlSchemeHandler(LocalHelpManager::instance());
    return schemeHandler;
}

WebEngineHelpViewer::WebEngineHelpViewer(QWidget *parent) :
    HelpViewer(parent),
    m_widget(new WebView(this))
{
    m_widget->setPage(new WebEngineHelpPage(this));
    auto layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_widget, 10);

    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::Text, Qt::black);
    setPalette(p);

    connect(m_widget, &QWebEngineView::urlChanged, this, &WebEngineHelpViewer::sourceChanged);
    connect(m_widget, &QWebEngineView::loadStarted, this, &WebEngineHelpViewer::slotLoadStarted);
    connect(m_widget, &QWebEngineView::loadFinished, this, &WebEngineHelpViewer::slotLoadFinished);
    connect(m_widget, &QWebEngineView::titleChanged, this, &WebEngineHelpViewer::titleChanged);
    connect(m_widget->page(), &QWebEnginePage::linkHovered, this, &WebEngineHelpViewer::setToolTip);
    connect(m_widget->pageAction(QWebEnginePage::Back), &QAction::changed, this, [this]() {
        emit backwardAvailable(isBackwardAvailable());
    });
    connect(m_widget->pageAction(QWebEnginePage::Forward), &QAction::changed, this, [this]() {
        emit forwardAvailable(isForwardAvailable());
    });

    QAction* action = m_widget->pageAction(QWebEnginePage::OpenLinkInNewTab);
    action->setText(QCoreApplication::translate("HelpViewer",
                                                Constants::TR_OPEN_LINK_AS_NEW_PAGE));

    QWebEnginePage *viewPage = m_widget->page();
    QTC_ASSERT(viewPage, return);
    QWebEngineProfile *viewProfile = viewPage->profile();
    QTC_ASSERT(viewProfile, return);
    if (!viewProfile->urlSchemeHandler("qthelp"))
        viewProfile->installUrlSchemeHandler("qthelp", helpUrlSchemeHandler());
}

QFont WebEngineHelpViewer::viewerFont() const
{
    QWebEngineSettings *webSettings = m_widget->settings();
    return QFont(webSettings->fontFamily(QWebEngineSettings::StandardFont),
                 webSettings->fontSize(QWebEngineSettings::DefaultFontSize));
}

void WebEngineHelpViewer::setViewerFont(const QFont &font)
{
    QWebEngineSettings *webSettings = m_widget->settings();
    webSettings->setFontFamily(QWebEngineSettings::StandardFont, font.family());
    webSettings->setFontSize(QWebEngineSettings::DefaultFontSize, font.pointSize());
}

qreal WebEngineHelpViewer::scale() const
{
    return m_widget->zoomFactor();
}

void WebEngineHelpViewer::setScale(qreal scale)
{
    m_widget->setZoomFactor(scale);
}

QString WebEngineHelpViewer::title() const
{
    return m_widget->title();
}

QUrl WebEngineHelpViewer::source() const
{
    return m_widget->url();
}

void WebEngineHelpViewer::setSource(const QUrl &url)
{
    m_widget->setUrl(url);
}

void WebEngineHelpViewer::setHtml(const QString &html)
{
    m_widget->setHtml(html);
}

QString WebEngineHelpViewer::selectedText() const
{
    return m_widget->selectedText();
}

bool WebEngineHelpViewer::isForwardAvailable() const
{
    // m_view->history()->canGoForward()
    return m_widget->pageAction(QWebEnginePage::Forward)->isEnabled();
}

bool WebEngineHelpViewer::isBackwardAvailable() const
{
    return m_widget->pageAction(QWebEnginePage::Back)->isEnabled();
}

void WebEngineHelpViewer::addBackHistoryItems(QMenu *backMenu)
{
    if (QWebEngineHistory *history = m_widget->history()) {
        QList<QWebEngineHistoryItem> items = history->backItems(history->count());
        for (int i = items.count() - 1; i >= 0; --i) {
            QWebEngineHistoryItem item = items.at(i);
            auto action = new QAction(backMenu);
            action->setText(item.title());
            connect(action, &QAction::triggered, this, [this,item]() {
                if (QWebEngineHistory *history = m_widget->history())
                    history->goToItem(item);
            });
            backMenu->addAction(action);
        }
    }
}

void WebEngineHelpViewer::addForwardHistoryItems(QMenu *forwardMenu)
{
    if (QWebEngineHistory *history = m_widget->history()) {
        QList<QWebEngineHistoryItem> items = history->forwardItems(history->count());
        for (int i = 0; i < items.count(); ++i) {
            QWebEngineHistoryItem item = items.at(i);
            auto action = new QAction(forwardMenu);
            action->setText(item.title());
            connect(action, &QAction::triggered, this, [this,item]() {
                if (QWebEngineHistory *history = m_widget->history())
                    history->goToItem(item);
            });
            forwardMenu->addAction(action);
        }
    }
}

bool WebEngineHelpViewer::findText(const QString &text, Core::FindFlags flags, bool incremental,
                                   bool fromSearch, bool *wrapped)
{
    Q_UNUSED(incremental)
    Q_UNUSED(fromSearch)
    if (wrapped)
        *wrapped = false; // missing feature in QWebEngine
    QWebEnginePage::FindFlags webEngineFlags = 0;
    if (flags & Core::FindBackward)
        webEngineFlags |= QWebEnginePage::FindBackward;
    if (flags & Core::FindCaseSensitively)
        webEngineFlags |= QWebEnginePage::FindCaseSensitively;
    // QWebEngineView's findText is asynchronous, and the variant taking a callback runs the
    // callback on the main thread, so blocking here becomes ugly too
    // So we just claim that the search succeeded
    m_widget->findText(text, webEngineFlags);
    return true;
}

WebEngineHelpPage *WebEngineHelpViewer::page() const
{
    return static_cast<WebEngineHelpPage *>(m_widget->page());
}

void WebEngineHelpViewer::scaleUp()
{
    m_widget->setZoomFactor(m_widget->zoomFactor() + 0.1);
}

void WebEngineHelpViewer::scaleDown()
{
    m_widget->setZoomFactor(qMax(qreal(0.1), m_widget->zoomFactor() - qreal(0.1)));
}

void WebEngineHelpViewer::resetScale()
{
    m_widget->setZoomFactor(1.0);
}

void WebEngineHelpViewer::copy()
{
    m_widget->triggerPageAction(QWebEnginePage::Copy);
}

void WebEngineHelpViewer::stop()
{
    m_widget->triggerPageAction(QWebEnginePage::Stop);
}

void WebEngineHelpViewer::forward()
{
    m_widget->triggerPageAction(QWebEnginePage::Forward);
}

void WebEngineHelpViewer::backward()
{
    m_widget->triggerPageAction(QWebEnginePage::Back);
}

void WebEngineHelpViewer::print(QPrinter *printer)
{
    Q_UNUSED(printer)
}

WebEngineHelpPage::WebEngineHelpPage(QObject *parent)
    : QWebEnginePage(parent)
{
}

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
QWebEnginePage *WebEngineHelpPage::createWindow(QWebEnginePage::WebWindowType)
{
    auto viewer = static_cast<WebEngineHelpViewer *>(OpenPagesManager::instance().createPage());
    return viewer->page();
}
#endif

WebView::WebView(WebEngineHelpViewer *viewer)
    : QWebEngineView(viewer),
      m_viewer(viewer)
{
}

void WebView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = page()->createStandardContextMenu();
    // insert Open as New Page etc if OpenLinkInThisWindow is also there
    const QList<QAction*> actions = menu->actions();
    auto it = std::find(actions.cbegin(), actions.cend(),
                        page()->action(QWebEnginePage::OpenLinkInThisWindow));
    if (it != actions.cend()) {
        // insert after
        ++it;
        QAction *before = (it == actions.cend() ? 0 : *it);
#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
        if (m_viewer->isActionVisible(HelpViewer::Action::NewPage)) {
            QAction *openLinkInNewTab = page()->action(QWebEnginePage::OpenLinkInNewTab);
            menu->insertAction(before, openLinkInNewTab);
        }
#else
        QUrl url = page()->contextMenuData().linkUrl();
        if (m_viewer->isActionVisible(HelpViewer::Action::NewPage)) {
            auto openLink = new QAction(QCoreApplication::translate("HelpViewer",
                                        Constants::TR_OPEN_LINK_AS_NEW_PAGE), menu);
            connect(openLink, &QAction::triggered, m_viewer, [this, url] {
                m_viewer->newPageRequested(url);
            });
            menu->insertAction(before, openLink);
        }
        if (m_viewer->isActionVisible(HelpViewer::Action::ExternalWindow)) {
            auto openLink = new QAction(QCoreApplication::translate("HelpViewer",
                                        Constants::TR_OPEN_LINK_IN_WINDOW), menu);
            connect(openLink, &QAction::triggered, m_viewer, [this, url] {
                m_viewer->externalPageRequested(url);
            });
            menu->insertAction(before, openLink);
        }
#endif
    }

    connect(menu, &QMenu::aboutToHide, menu, &QObject::deleteLater);
    menu->popup(event->globalPos());
}
