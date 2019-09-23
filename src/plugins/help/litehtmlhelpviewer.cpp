/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "litehtmlhelpviewer.h"

#include "helpconstants.h"
#include "localhelpmanager.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <QDebug>

using namespace Help;
using namespace Help::Internal;

const int kMaxHistoryItems = 20;

static QByteArray getData(const QUrl &url)
{
    // TODO: this is just a hack for Qt documentation
    // which decides to use a simpler CSS if the viewer does not have JavaScript
    // which was a hack to decide if we are viewing in QTextBrowser or QtWebEngine et al
    QUrl actualUrl = url;
    QString path = url.path(QUrl::FullyEncoded);
    static const char simpleCss[] = "/offline-simple.css";
    if (path.endsWith(simpleCss)) {
        path.replace(simpleCss, "/offline.css");
        actualUrl.setPath(path);
    }
    const LocalHelpManager::HelpData help = LocalHelpManager::helpData(actualUrl);
    return help.data;
}

LiteHtmlHelpViewer::LiteHtmlHelpViewer(QWidget *parent)
    : HelpViewer(parent)
    , m_viewer(new QLiteHtmlWidget)
{
    m_viewer->setResourceHandler([](const QUrl &url) { return getData(url); });
    m_viewer->viewport()->installEventFilter(this);
    connect(m_viewer, &QLiteHtmlWidget::linkClicked, this, &LiteHtmlHelpViewer::setSource);
    connect(m_viewer,
            &QLiteHtmlWidget::contextMenuRequested,
            this,
            &LiteHtmlHelpViewer::showContextMenu);
    auto layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_viewer, 10);
    setFocusProxy(m_viewer);
    QPalette p = palette();
    p.setColor(QPalette::Inactive, QPalette::Highlight,
        p.color(QPalette::Active, QPalette::Highlight));
    p.setColor(QPalette::Inactive, QPalette::HighlightedText,
        p.color(QPalette::Active, QPalette::HighlightedText));
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::Text, Qt::black);
    setPalette(p);
}

LiteHtmlHelpViewer::~LiteHtmlHelpViewer() = default;

QFont LiteHtmlHelpViewer::viewerFont() const
{
    return m_viewer->defaultFont();
}

void LiteHtmlHelpViewer::setViewerFont(const QFont &newFont)
{
    m_viewer->setDefaultFont(newFont);
}

void LiteHtmlHelpViewer::scaleUp()
{
    setScale(scale() * 1.1);
}

void LiteHtmlHelpViewer::scaleDown()
{
    setScale(scale() * .9);
}

void LiteHtmlHelpViewer::resetScale()
{
    m_viewer->setZoomFactor(1);
}

qreal LiteHtmlHelpViewer::scale() const
{
    return m_viewer->zoomFactor();
}

void LiteHtmlHelpViewer::setScale(qreal scale)
{
    // interpret 0 as "default"
    m_viewer->setZoomFactor(scale == 0 ? qreal(1) : scale);
}

QString LiteHtmlHelpViewer::title() const
{
    return m_viewer->title();
}

QUrl LiteHtmlHelpViewer::source() const
{
    return m_viewer->url();
}

void LiteHtmlHelpViewer::setSource(const QUrl &url)
{
    if (launchWithExternalApp(url))
        return;
    m_forwardItems.clear();
    emit forwardAvailable(false);
    if (m_viewer->url().isValid()) {
        m_backItems.push_back(currentHistoryItem());
        while (m_backItems.size() > kMaxHistoryItems) // this should trigger only once anyhow
            m_backItems.erase(m_backItems.begin());
        emit backwardAvailable(true);
    }
    setSourceInternal(url);
}

void LiteHtmlHelpViewer::setHtml(const QString &html)
{
    m_viewer->setUrl({"about:invalid"});
    m_viewer->setHtml(html);
}

QString LiteHtmlHelpViewer::selectedText() const
{
    return m_viewer->selectedText();
}

bool LiteHtmlHelpViewer::isForwardAvailable() const
{
    return !m_forwardItems.empty();
}

bool LiteHtmlHelpViewer::isBackwardAvailable() const
{
    return !m_backItems.empty();
}

void LiteHtmlHelpViewer::addBackHistoryItems(QMenu *backMenu)
{
    int backCount = 0;
    Utils::reverseForeach(m_backItems, [this, backMenu, &backCount](const HistoryItem &item) {
        ++backCount;
        auto action = new QAction(backMenu);
        action->setText(item.title);
        connect(action, &QAction::triggered, this, [this, backCount] { goBackward(backCount); });
        backMenu->addAction(action);
    });
}

void LiteHtmlHelpViewer::addForwardHistoryItems(QMenu *forwardMenu)
{
    int forwardCount = 0;
    for (const HistoryItem &item : m_forwardItems) {
        ++forwardCount;
        auto action = new QAction(forwardMenu);
        action->setText(item.title);
        connect(action, &QAction::triggered, this, [this, forwardCount] {
            goForward(forwardCount);
        });
        forwardMenu->addAction(action);
    }
}

bool LiteHtmlHelpViewer::findText(
    const QString &text, Core::FindFlags flags, bool incremental, bool fromSearch, bool *wrapped)
{
    return m_viewer->findText(text,
                              Core::textDocumentFlagsForFindFlags(flags),
                              incremental,
                              wrapped);
}

void LiteHtmlHelpViewer::copy()
{
    QGuiApplication::clipboard()->setText(selectedText());
}

void LiteHtmlHelpViewer::stop() {}

void LiteHtmlHelpViewer::forward()
{
    goForward(1);
}

void LiteHtmlHelpViewer::backward()
{
    goBackward(1);
}
void LiteHtmlHelpViewer::goForward(int count)
{
    HistoryItem nextItem = currentHistoryItem();
    for (int i = 0; i < count; ++i) {
        QTC_ASSERT(!m_forwardItems.empty(), return );
        m_backItems.push_back(nextItem);
        nextItem = m_forwardItems.front();
        m_forwardItems.erase(m_forwardItems.begin());
    }
    emit backwardAvailable(isBackwardAvailable());
    emit forwardAvailable(isForwardAvailable());
    setSourceInternal(nextItem.url, nextItem.vscroll);
}

void LiteHtmlHelpViewer::goBackward(int count)
{
    HistoryItem previousItem = currentHistoryItem();
    for (int i = 0; i < count; ++i) {
        QTC_ASSERT(!m_backItems.empty(), return );
        m_forwardItems.insert(m_forwardItems.begin(), previousItem);
        previousItem = m_backItems.back();
        m_backItems.pop_back();
    }
    emit backwardAvailable(isBackwardAvailable());
    emit forwardAvailable(isForwardAvailable());
    setSourceInternal(previousItem.url, previousItem.vscroll);
}

void LiteHtmlHelpViewer::print(QPrinter *printer)
{
    // TODO
}

bool LiteHtmlHelpViewer::eventFilter(QObject *src, QEvent *e)
{
    if (isScrollWheelZoomingEnabled() && e->type() == QEvent::Wheel) {
        auto we = static_cast<QWheelEvent *>(e);
        if (we->modifiers() == Qt::ControlModifier)
            return true;
    }
    return HelpViewer::eventFilter(src, e);
}

void LiteHtmlHelpViewer::setSourceInternal(const QUrl &url, Utils::optional<int> vscroll)
{
    slotLoadStarted();
    QUrl currentUrlWithoutFragment = m_viewer->url();
    currentUrlWithoutFragment.setFragment({});
    QUrl newUrlWithoutFragment = url;
    newUrlWithoutFragment.setFragment({});
    m_viewer->setUrl(url);
    if (currentUrlWithoutFragment != newUrlWithoutFragment)
        m_viewer->setHtml(QString::fromUtf8(getData(url)));
    if (vscroll)
        m_viewer->verticalScrollBar()->setValue(*vscroll);
    else
        m_viewer->scrollToAnchor(url.fragment(QUrl::FullyEncoded));
    slotLoadFinished();
    emit titleChanged();
}

void LiteHtmlHelpViewer::showContextMenu(const QPoint &pos, const QUrl &url)
{
    QMenu menu(nullptr);

    QAction *copyAnchorAction = nullptr;
    if (!url.isEmpty() && url.isValid()) {
        if (isActionVisible(HelpViewer::Action::NewPage)) {
            QAction *action = menu.addAction(
                QCoreApplication::translate("HelpViewer", Constants::TR_OPEN_LINK_AS_NEW_PAGE));
            connect(action, &QAction::triggered, this, [this, url]() {
                emit newPageRequested(url);
            });
        }
        if (isActionVisible(HelpViewer::Action::ExternalWindow)) {
            QAction *action = menu.addAction(
                QCoreApplication::translate("HelpViewer", Constants::TR_OPEN_LINK_IN_WINDOW));
            connect(action, &QAction::triggered, this, [this, url]() {
                emit externalPageRequested(url);
            });
        }
        copyAnchorAction = menu.addAction(tr("Copy Link"));
    } else if (!m_viewer->selectedText().isEmpty()) {
        connect(menu.addAction(tr("Copy")), &QAction::triggered, this, &HelpViewer::copy);
    }

    if (copyAnchorAction == menu.exec(m_viewer->mapToGlobal(pos)))
        QGuiApplication::clipboard()->setText(url.toString());
}

LiteHtmlHelpViewer::HistoryItem LiteHtmlHelpViewer::currentHistoryItem() const
{
    return {m_viewer->url(), m_viewer->title(), m_viewer->verticalScrollBar()->value()};
}

//bool TextBrowserHelpWidget::eventFilter(QObject *obj, QEvent *event)
//{
//    if (obj == this) {
//        if (event->type() == QEvent::FontChange) {
//            if (!forceFont)
//                return true;
//        } else if (event->type() == QEvent::KeyPress) {
//            auto keyEvent = static_cast<QKeyEvent *>(event);
//            if (keyEvent->key() == Qt::Key_Slash) {
//                keyEvent->accept();
//                Core::Find::openFindToolBar(Core::Find::FindForwardDirection);
//                return true;
//            }
//        } else if (event->type() == QEvent::ToolTip) {
//            auto e = static_cast<const QHelpEvent *>(event);
//            QToolTip::showText(e->globalPos(), linkAt(e->pos()));
//            return true;
//        }
//    }
//    return QTextBrowser::eventFilter(obj, event);
//}

//void TextBrowserHelpWidget::mousePressEvent(QMouseEvent *e)
//{
//    if (Utils::HostOsInfo::isLinuxHost() && m_parent->handleForwardBackwardMouseButtons(e))
//        return;
//    QTextBrowser::mousePressEvent(e);
//}

//void TextBrowserHelpWidget::mouseReleaseEvent(QMouseEvent *e)
//{
//    if (!Utils::HostOsInfo::isLinuxHost() && m_parent->handleForwardBackwardMouseButtons(e))
//        return;

//    bool controlPressed = e->modifiers() & Qt::ControlModifier;
//    const QString link = linkAt(e->pos());
//    if (m_parent->isActionVisible(HelpViewer::Action::NewPage)
//            && (controlPressed || e->button() == Qt::MidButton) && !link.isEmpty()) {
//        emit m_parent->newPageRequested(QUrl(link));
//        return;
//    }

//    QTextBrowser::mouseReleaseEvent(e);
//}
