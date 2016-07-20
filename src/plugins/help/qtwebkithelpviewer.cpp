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

#include "qtwebkithelpviewer.h"

#if !defined(QT_NO_WEBKIT)

#include "helpconstants.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"

#include <coreplugin/find/findplugin.h>
#include <utils/hostosinfo.h>
#include <utils/networkaccessmanager.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QDebug>
#include <QString>
#include <QStringBuilder>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebElement>
#include <QWebFrame>
#include <QWebHistory>

#include <QApplication>
#include <QDesktopServices>
#include <QWheelEvent>

#include <QHelpEngine>

#include <QNetworkReply>
#include <QNetworkRequest>


#include <cstring>

using namespace Core;
using namespace Help;
using namespace Help::Internal;

// -- HelpNetworkReply

class HelpNetworkReply : public QNetworkReply
{
public:
    HelpNetworkReply(const QNetworkRequest &request, const QByteArray &fileData,
        const QString &mimeType);

    virtual void abort() {}

    virtual qint64 bytesAvailable() const
        { return data.length() + QNetworkReply::bytesAvailable(); }

protected:
    virtual qint64 readData(char *data, qint64 maxlen);

private:
    QByteArray data;
    qint64 dataLength;
};

HelpNetworkReply::HelpNetworkReply(const QNetworkRequest &request,
        const QByteArray &fileData, const QString& mimeType)
    : data(fileData)
    , dataLength(fileData.length())
{
    setRequest(request);
    setUrl(request.url());
    setOpenMode(QIODevice::ReadOnly);

    setHeader(QNetworkRequest::ContentTypeHeader, mimeType);
    setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(dataLength));
    QTimer::singleShot(0, this, &QNetworkReply::metaDataChanged);
    QTimer::singleShot(0, this, &QIODevice::readyRead);
    QTimer::singleShot(0, this, &QNetworkReply::finished);
}

qint64 HelpNetworkReply::readData(char *buffer, qint64 maxlen)
{
    qint64 len = qMin(qint64(data.length()), maxlen);
    if (len) {
        std::memcpy(buffer, data.constData(), len);
        data.remove(0, len);
    }
    return len;
}

// -- HelpNetworkAccessManager

class HelpNetworkAccessManager : public Utils::NetworkAccessManager
{
public:
    HelpNetworkAccessManager(QObject *parent);

protected:
    virtual QNetworkReply *createRequest(Operation op,
        const QNetworkRequest &request, QIODevice *outgoingData = 0);
};

HelpNetworkAccessManager::HelpNetworkAccessManager(QObject *parent)
    : Utils::NetworkAccessManager(parent)
{
}

QNetworkReply *HelpNetworkAccessManager::createRequest(Operation op,
    const QNetworkRequest &request, QIODevice* outgoingData)
{
    if (!HelpViewer::isLocalUrl(request.url()))
        return Utils::NetworkAccessManager::createRequest(op, request, outgoingData);

    LocalHelpManager::HelpData data = LocalHelpManager::helpData(request.url());
    return new HelpNetworkReply(request, data.data, data.mimeType);
}

// - QtWebKitHelpPage

QtWebKitHelpPage::QtWebKitHelpPage(QObject *parent)
    : QWebPage(parent)
    , closeNewTabIfNeeded(false)
    , m_pressedButtons(Qt::NoButton)
    , m_keyboardModifiers(Qt::NoModifier)
{
    setForwardUnsupportedContent(true);
    connect(this, &QWebPage::unsupportedContent, this,
        &QtWebKitHelpPage::onHandleUnsupportedContent);
}

QWebPage *QtWebKitHelpPage::createWindow(QWebPage::WebWindowType)
{
    // TODO: ensure that we'll get a QtWebKitHelpViewer here
    QtWebKitHelpViewer* viewer = static_cast<QtWebKitHelpViewer *>(OpenPagesManager::instance()
        .createPage());
    QtWebKitHelpPage *newPage = viewer->page();
    newPage->closeNewTabIfNeeded = closeNewTabIfNeeded;
    closeNewTabIfNeeded = false;
    return newPage;
}

void QtWebKitHelpPage::triggerAction(WebAction action, bool checked)
{
    switch (action) {
        case OpenLinkInNewWindow:
            closeNewTabIfNeeded = true;
        default:        // fall through
            QWebPage::triggerAction(action, checked);
            break;
    }
}

bool QtWebKitHelpPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request,
    QWebPage::NavigationType type)
{
    const bool closeNewTab = closeNewTabIfNeeded;
    closeNewTabIfNeeded = false;

    const QUrl &url = request.url();
    if (HelpViewer::launchWithExternalApp(url)) {
        if (closeNewTab)
            QMetaObject::invokeMethod(&OpenPagesManager::instance(), "closeCurrentPage");
        return false;
    }

    if (type == QWebPage::NavigationTypeLinkClicked
        && (m_keyboardModifiers & Qt::ControlModifier || m_pressedButtons == Qt::MidButton)) {
            m_pressedButtons = Qt::NoButton;
            m_keyboardModifiers = Qt::NoModifier;
            OpenPagesManager::instance().createPage(url);
            return false;
    }

    if (frame == mainFrame())
        m_loadingUrl = request.url();

    return true;
}

void QtWebKitHelpPage::onHandleUnsupportedContent(QNetworkReply *reply)
{
    // sub resource of this page
    if (m_loadingUrl != reply->url()) {
        qWarning() << "Resource" << reply->url().toEncoded() << "has unknown Content-Type, will be ignored.";
        reply->deleteLater();
        return;
    }

    // set a default error string we are going to display
    QString errorString = HelpViewer::tr("Unknown or unsupported content.");
    if (reply->error() == QNetworkReply::NoError) {
        // try to open the url using using the desktop service
        if (QDesktopServices::openUrl(reply->url())) {
            reply->deleteLater();
            return;
        }
        // seems we failed, now we show the error page inside creator
    } else {
        errorString = reply->errorString();
    }

    const QString html = QString::fromUtf8(LocalHelpManager::loadErrorMessage(reply->url(),
                                                                              errorString));

    // update the current layout
    QList<QWebFrame*> frames;
    frames.append(mainFrame());
    while (!frames.isEmpty()) {
        QWebFrame *frame = frames.takeFirst();
        if (frame->url() == reply->url()) {
            frame->setHtml(html, reply->url());
            return;
        }

        QList<QWebFrame *> children = frame->childFrames();
        foreach (QWebFrame *frame, children)
            frames.append(frame);
    }

    if (m_loadingUrl == reply->url())
        mainFrame()->setHtml(html, reply->url());
}


// -- HelpViewer

QtWebKitHelpWidget::QtWebKitHelpWidget(QtWebKitHelpViewer *parent)
    : QWebView(parent),
      m_parent(parent)
{
    setAcceptDrops(false);
    installEventFilter(this);

    QWebSettings::globalSettings()->setAttribute(QWebSettings::DnsPrefetchEnabled, true);

    setPage(new QtWebKitHelpPage(this));
    HelpNetworkAccessManager *manager = new HelpNetworkAccessManager(this);
    page()->setNetworkAccessManager(manager);
    connect(manager, &QNetworkAccessManager::finished, this,
        &QtWebKitHelpWidget::slotNetworkReplyFinished);

    QAction* action = pageAction(QWebPage::OpenLinkInNewWindow);
    action->setText(tr("Open Link as New Page"));

    pageAction(QWebPage::DownloadLinkToDisk)->setVisible(false);
    pageAction(QWebPage::DownloadImageToDisk)->setVisible(false);
    pageAction(QWebPage::OpenImageInNewWindow)->setVisible(false);

    connect(pageAction(QWebPage::Copy), &QAction::changed, this,
        &QtWebKitHelpWidget::actionChanged);
    connect(pageAction(QWebPage::Back), &QAction::changed, this,
        &QtWebKitHelpWidget::actionChanged);
    connect(pageAction(QWebPage::Forward), &QAction::changed, this,
        &QtWebKitHelpWidget::actionChanged);
}

QtWebKitHelpWidget::~QtWebKitHelpWidget()
{
}

void QtWebKitHelpWidget::scaleUp()
{
    setZoomFactor(zoomFactor() + 0.1);
}

void QtWebKitHelpWidget::scaleDown()
{
    setZoomFactor(qMax(qreal(0.0), zoomFactor() - qreal(0.1)));
}

void QtWebKitHelpWidget::copy()
{
    triggerPageAction(QWebPage::Copy);
}

// -- protected

void QtWebKitHelpWidget::keyPressEvent(QKeyEvent *e)
{
    // TODO: remove this once we support multiple keysequences per command
    if (e->key() == Qt::Key_Insert && e->modifiers() == Qt::CTRL) {
        if (!selectedText().isEmpty())
            copy();
    }
    QWebView::keyPressEvent(e);
}

void QtWebKitHelpWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers()& Qt::ControlModifier) {
        event->accept();
        event->delta() > 0 ? scaleUp() : scaleDown();
    } else {
        QWebView::wheelEvent(event);
    }
}

void QtWebKitHelpWidget::mousePressEvent(QMouseEvent *event)
{
    if (Utils::HostOsInfo::isLinuxHost() && m_parent->handleForwardBackwardMouseButtons(event))
        return;

    if (QtWebKitHelpPage *currentPage = static_cast<QtWebKitHelpPage*>(page())) {
        currentPage->m_pressedButtons = event->buttons();
        currentPage->m_keyboardModifiers = event->modifiers();
    }

    QWebView::mousePressEvent(event);
}

void QtWebKitHelpWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!Utils::HostOsInfo::isLinuxHost() && m_parent->handleForwardBackwardMouseButtons(event))
        return;

    QWebView::mouseReleaseEvent(event);
}

void QtWebKitHelpWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QAction *newPageAction = pageAction(QWebPage::OpenLinkInNewWindow);
    newPageAction->setText(QCoreApplication::translate("HelpViewer", "Open Link as New Page"));
    QMenu *menu = page()->createStandardContextMenu();
    menu->exec(event->globalPos());
    delete menu;
}

// -- private

void QtWebKitHelpWidget::actionChanged()
{
    QAction *a = qobject_cast<QAction *>(sender());
    if (a == pageAction(QWebPage::Back))
        emit backwardAvailable(a->isEnabled());
    else if (a == pageAction(QWebPage::Forward))
        emit forwardAvailable(a->isEnabled());
}

void QtWebKitHelpWidget::slotNetworkReplyFinished(QNetworkReply *reply)
{
    if (reply && reply->error() != QNetworkReply::NoError) {
        load(QUrl(Help::Constants::AboutBlank));
        setHtml(QString::fromUtf8(LocalHelpManager::loadErrorMessage(reply->url(),
                                                                     reply->errorString())));
    }
}

// -- private

bool QtWebKitHelpWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        if (QKeyEvent *keyEvent = static_cast<QKeyEvent*> (event)) {
            if (keyEvent->key() == Qt::Key_Slash)
                Find::openFindToolBar(Find::FindForwardDirection);
        }
    }
    return QWebView::eventFilter(obj, event);
}

QtWebKitHelpViewer::QtWebKitHelpViewer(QWidget *parent)
    : HelpViewer(parent),
      m_webView(new QtWebKitHelpWidget(this))
{
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_webView, 10);

    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::Text, Qt::black);
    setPalette(p);

    connect(m_webView, &QWebView::urlChanged, this, &HelpViewer::sourceChanged);
    connect(m_webView, &QWebView::loadStarted, this, &QtWebKitHelpViewer::slotLoadStarted);
    connect(m_webView, &QWebView::loadFinished, this, &QtWebKitHelpViewer::slotLoadFinished);
    connect(m_webView, &QWebView::titleChanged, this, &HelpViewer::titleChanged);
    connect(m_webView->page(), &QWebPage::printRequested, this, &HelpViewer::printRequested);
    connect(m_webView, &QtWebKitHelpWidget::backwardAvailable, this, &HelpViewer::backwardAvailable);
    connect(m_webView, &QtWebKitHelpWidget::forwardAvailable, this, &HelpViewer::forwardAvailable);
    connect(page(), &QtWebKitHelpPage::linkHovered, this, &QtWebKitHelpViewer::setToolTip);
}

QFont QtWebKitHelpViewer::viewerFont() const
{
    QWebSettings* webSettings = m_webView->settings();
    return QFont(webSettings->fontFamily(QWebSettings::StandardFont),
        webSettings->fontSize(QWebSettings::DefaultFontSize));
}

void QtWebKitHelpViewer::setViewerFont(const QFont &font)
{
    QWebSettings *webSettings = m_webView->settings();
    webSettings->setFontFamily(QWebSettings::StandardFont, font.family());
    webSettings->setFontSize(QWebSettings::DefaultFontSize, font.pointSize());
}

void QtWebKitHelpViewer::scaleUp()
{
    m_webView->scaleUp();
}

void QtWebKitHelpViewer::scaleDown()
{
    m_webView->scaleDown();
}

void QtWebKitHelpViewer::resetScale()
{
    m_webView->setZoomFactor(1.0);
}

qreal QtWebKitHelpViewer::scale() const
{
    return m_webView->zoomFactor();
}

void QtWebKitHelpViewer::setScale(qreal scale)
{
    m_webView->setZoomFactor(scale <= 0.0 ? 1.0 : scale);
}

QString QtWebKitHelpViewer::title() const
{
    return m_webView->title();
}

QUrl QtWebKitHelpViewer::source() const
{
    return m_webView->url();
}

void QtWebKitHelpViewer::setSource(const QUrl &url)
{
    QUrl oldWithoutFragment = source();
    oldWithoutFragment.setFragment(QString());

    m_webView->load(url);

    // if the new url only changes the anchor,
    // then webkit does not send loadStarted nor loadFinished,
    // so we should do that manually in that case
    QUrl newWithoutFragment = url;
    newWithoutFragment.setFragment(QString());
    if (oldWithoutFragment == newWithoutFragment) {
        m_webView->page()->mainFrame()->scrollToAnchor(url.fragment());
        slotLoadStarted();
        slotLoadFinished();
    }
}

void QtWebKitHelpViewer::highlightId(const QString &id)
{
    if (m_oldHighlightId == id)
        return;
    const QWebElement &document = m_webView->page()->mainFrame()->documentElement();
    const QWebElementCollection &collection = document.findAll("h3.fn a");

    const QLatin1String property("background-color");
    foreach (const QWebElement &element, collection) {
        const QString &name = element.attribute("name");
        if (name.isEmpty())
            continue;

        if (m_oldHighlightId == name
                || name.startsWith(m_oldHighlightId + QLatin1Char('-'))) {
            QWebElement parent = element.parent();
            parent.setStyleProperty(property, m_oldHighlightStyle);
        }

        if (id == name
                || name.startsWith(id + QLatin1Char('-'))) {
            QWebElement parent = element.parent();
            m_oldHighlightStyle = parent.styleProperty(property,
                                                   QWebElement::ComputedStyle);
            parent.setStyleProperty(property, "yellow");
        }
    }
    m_oldHighlightId = id;
}

void QtWebKitHelpViewer::setHtml(const QString &html)
{
    m_webView->setHtml(html);
}

QString QtWebKitHelpViewer::selectedText() const
{
    return m_webView->selectedText();
}

bool QtWebKitHelpViewer::isForwardAvailable() const
{
    return m_webView->pageAction(QWebPage::Forward)->isEnabled();
}

bool QtWebKitHelpViewer::isBackwardAvailable() const
{
    return m_webView->pageAction(QWebPage::Back)->isEnabled();
}

void QtWebKitHelpViewer::addBackHistoryItems(QMenu *backMenu)
{
    if (QWebHistory *history = m_webView->history()) {
        QList<QWebHistoryItem> items = history->backItems(history->count());
        for (int i = items.count() - 1; i >= 0; --i) {
            QAction *action = new QAction(backMenu);
            action->setText(items.at(i).title());
            action->setData(i);
            connect(action, &QAction::triggered, this, &QtWebKitHelpViewer::goToBackHistoryItem);
            backMenu->addAction(action);
        }
    }
}

void QtWebKitHelpViewer::addForwardHistoryItems(QMenu *forwardMenu)
{
    if (QWebHistory *history = m_webView->history()) {
        QList<QWebHistoryItem> items = history->forwardItems(history->count());
        for (int i = 0; i < items.count(); ++i) {
            QAction *action = new QAction(forwardMenu);
            action->setText(items.at(i).title());
            action->setData(i);
            connect(action, &QAction::triggered, this, &QtWebKitHelpViewer::goToForwardHistoryItem);
            forwardMenu->addAction(action);
        }
    }
}

void QtWebKitHelpViewer::setOpenInNewPageActionVisible(bool visible)
{
    m_webView->pageAction(QWebPage::OpenLinkInNewWindow)->setVisible(visible);
}

bool QtWebKitHelpViewer::findText(const QString &text, FindFlags flags,
    bool incremental, bool fromSearch, bool *wrapped)
{
    Q_UNUSED(incremental);
    Q_UNUSED(fromSearch);
    if (wrapped)
        *wrapped = false;
    QWebPage::FindFlags options;
    if (flags & FindBackward)
        options |= QWebPage::FindBackward;
    if (flags & FindCaseSensitively)
        options |= QWebPage::FindCaseSensitively;

    bool found = m_webView->findText(text, options);
    if (!found) {
        options |= QWebPage::FindWrapsAroundDocument;
        found = m_webView->findText(text, options);
        if (found && wrapped)
            *wrapped = true;
    }
    options = QWebPage::HighlightAllOccurrences;
    m_webView->findText("", options); // clear first
    m_webView->findText(text, options); // force highlighting of all other matches
    return found;
}

QtWebKitHelpPage *QtWebKitHelpViewer::page() const
{
    return static_cast<QtWebKitHelpPage *>(m_webView->page());
}

void QtWebKitHelpViewer::copy()
{
    m_webView->copy();
}

void QtWebKitHelpViewer::stop()
{
    m_webView->triggerPageAction(QWebPage::Stop);
}

void QtWebKitHelpViewer::forward()
{
    m_webView->forward();
}

void QtWebKitHelpViewer::backward()
{
    m_webView->back();
}

void QtWebKitHelpViewer::print(QPrinter *printer)
{
    m_webView->print(printer);
}

void QtWebKitHelpViewer::goToBackHistoryItem()
{
    goToHistoryItem(/*forward=*/false);
}

void QtWebKitHelpViewer::goToForwardHistoryItem()
{
    goToHistoryItem(/*forward=*/true);
}

void QtWebKitHelpViewer::goToHistoryItem(bool forward)
{
    QAction *action = qobject_cast<QAction *>(sender());
    QTC_ASSERT(action, return);
    QWebHistory *history = m_webView->history();
    QTC_ASSERT(history, return);
    bool ok = false;
    int index = action->data().toInt(&ok);
    QTC_ASSERT(ok, return);
    if (forward)
        history->goToItem(history->forwardItems(history->count()).at(index));
    else
        history->goToItem(history->backItems(history->count()).at(index));
}

#endif  // !QT_NO_WEBKIT
