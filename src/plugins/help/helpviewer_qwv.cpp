/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "helpviewer.h"

#if !defined(QT_NO_WEBKIT)

#include "centralwidget.h"
#include "helpconstants.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"

#include <utils/hostosinfo.h>

#include <QDebug>
#include <QFileInfo>
#include <QString>
#include <QStringBuilder>
#include <QTimer>
#include <QWebFrame>

#include <QApplication>
#include <QDesktopServices>
#include <QWheelEvent>

#include <QHelpEngine>

#include <QNetworkReply>
#include <QNetworkRequest>

#include <utils/networkaccessmanager.h>

#include <cstring>

using namespace Find;
using namespace Help;
using namespace Help::Internal;

static const char g_htmlPage[] = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; "
    "charset=UTF-8\"><title>%1</title><style>body{padding: 3em 0em;background: #eeeeee;}"
    "hr{color: lightgray;width: 100%;}img{float: left;opacity: .8;}#box{background: white;border: 1px solid "
    "lightgray;width: 600px;padding: 60px;margin: auto;}h1{font-size: 130%;font-weight: bold;border-bottom: "
    "1px solid lightgray;margin-left: 48px;}h2{font-size: 100%;font-weight: normal;border-bottom: 1px solid "
    "lightgray;margin-left: 48px;}ul{font-size: 80%;padding-left: 48px;margin: 0;}#reloadButton{padding-left:"
    "48px;}</style></head><body><div id=\"box\"><img src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACA"
    "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\" width=\"32\"height=\"32\" /><h1>%2</h1><h2>%3</h2>"
    "<ul>%4%5%6%7</ul></div></body></html>";

// some of the values we will replace %1...6 inside the former html
const QString g_percent1 = QCoreApplication::translate("HelpViewer", "Error 404...");
const QString g_percent2 = QCoreApplication::translate("HelpViewer", "The page could not be found!");
// percent3 will be the url of the page we got the error from
const QString g_percent4 = QCoreApplication::translate("HelpViewer", "<li>Check that you have one or more "
    "documentation sets installed.</li>");
const QString g_percent5 = QCoreApplication::translate("HelpViewer", "<li>Check that you have installed the "
    "appropriate browser plug-in to support the file your loading.</li>");
const QString g_percent6 = QCoreApplication::translate("HelpViewer", "<li>If you try to access a public "
    "URL, make sure to have a network connection.</li>");
const QString g_percent7 = QCoreApplication::translate("HelpViewer", "<li>If your computer or network is "
    "protected by a firewall or proxy, make sure the application is permitted to access the network.</li>");


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
    QTimer::singleShot(0, this, SIGNAL(metaDataChanged()));
    QTimer::singleShot(0, this, SIGNAL(readyRead()));
    QTimer::singleShot(0, this, SIGNAL(finished()));
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

    QString url = request.url().toString();
    const QHelpEngineCore &engine = LocalHelpManager::helpEngine();

    const QString &mimeType = HelpViewer::mimeFromUrl(url);
    const QByteArray &data = engine.findFile(url).isValid() ? engine.fileData(url)
        : QString::fromLatin1(g_htmlPage).arg(g_percent1, g_percent2, HelpViewer::tr("Error loading: %1")
            .arg(url), g_percent4, g_percent6, g_percent7, QString()).toUtf8();

    return new HelpNetworkReply(request, data, mimeType.isEmpty()
        ? QLatin1String("application/octet-stream") : mimeType);
}

// - HelpPage

HelpPage::HelpPage(QObject *parent)
    : QWebPage(parent)
    , closeNewTabIfNeeded(false)
    , m_pressedButtons(Qt::NoButton)
    , m_keyboardModifiers(Qt::NoModifier)
{
    setForwardUnsupportedContent(true);
    connect(this, SIGNAL(unsupportedContent(QNetworkReply*)), this,
        SLOT(onHandleUnsupportedContent(QNetworkReply*)));
}

QWebPage *HelpPage::createWindow(QWebPage::WebWindowType)
{
    HelpPage* newPage = static_cast<HelpPage*>(OpenPagesManager::instance()
        .createPage()->page());
    newPage->closeNewTabIfNeeded = closeNewTabIfNeeded;
    closeNewTabIfNeeded = false;
    return newPage;
}

void HelpPage::triggerAction(WebAction action, bool checked)
{
    switch (action) {
        case OpenLinkInNewWindow:
            closeNewTabIfNeeded = true;
        default:        // fall through
            QWebPage::triggerAction(action, checked);
            break;
    }
}

bool HelpPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request,
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

void HelpPage::onHandleUnsupportedContent(QNetworkReply *reply)
{
    // sub resource of this page
    if (m_loadingUrl != reply->url()) {
        qWarning() << "Resource" << reply->url().toEncoded() << "has unknown Content-Type, will be ignored.";
        reply->deleteLater();
        return;
    }

    // set a default error string we are going to display
    QString errorString = HelpViewer::tr("Unknown or unsupported Content!");
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

    // setup html
    const QString html = QString::fromLatin1(g_htmlPage).arg(g_percent1, errorString,
        HelpViewer::tr("Error loading: %1").arg(reply->url().toString()), g_percent4, g_percent5, g_percent6,
        g_percent7);

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

HelpViewer::HelpViewer(qreal zoom, QWidget *parent)
    : QWebView(parent)
{
    setAcceptDrops(false);
    installEventFilter(this);

    QWebSettings::globalSettings()->setAttribute(QWebSettings::DnsPrefetchEnabled, true);

    setPage(new HelpPage(this));
    HelpNetworkAccessManager *manager = new HelpNetworkAccessManager(this);
    page()->setNetworkAccessManager(manager);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this,
        SLOT(slotNetworkReplyFinished(QNetworkReply*)));

    QAction* action = pageAction(QWebPage::OpenLinkInNewWindow);
    action->setText(tr("Open Link as New Page"));

    pageAction(QWebPage::DownloadLinkToDisk)->setVisible(false);
    pageAction(QWebPage::DownloadImageToDisk)->setVisible(false);
    pageAction(QWebPage::OpenImageInNewWindow)->setVisible(false);

    connect(pageAction(QWebPage::Copy), SIGNAL(changed()), this,
        SLOT(actionChanged()));
    connect(pageAction(QWebPage::Back), SIGNAL(changed()), this,
        SLOT(actionChanged()));
    connect(pageAction(QWebPage::Forward), SIGNAL(changed()), this,
        SLOT(actionChanged()));
    connect(this, SIGNAL(urlChanged(QUrl)), this, SIGNAL(sourceChanged(QUrl)));
    connect(this, SIGNAL(loadStarted()), this, SLOT(slotLoadStarted()));
    connect(this, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished(bool)));
    connect(this, SIGNAL(titleChanged(QString)), this, SIGNAL(titleChanged()));
    connect(page(), SIGNAL(printRequested(QWebFrame*)), this, SIGNAL(printRequested()));

    setViewerFont(viewerFont());
    setZoomFactor(zoom == 0.0 ? 1.0 : zoom);
}

HelpViewer::~HelpViewer()
{
}

QFont HelpViewer::viewerFont() const
{
    QWebSettings* webSettings = QWebSettings::globalSettings();
    QFont font(QApplication::font().family(),
        webSettings->fontSize(QWebSettings::DefaultFontSize));
    const QHelpEngineCore &engine = LocalHelpManager::helpEngine();
    return qvariant_cast<QFont>(engine.customValue(QLatin1String("font"),
        font));
}

void HelpViewer::setViewerFont(const QFont &font)
{
    QWebSettings *webSettings = settings();
    webSettings->setFontFamily(QWebSettings::StandardFont, font.family());
    webSettings->setFontSize(QWebSettings::DefaultFontSize, font.pointSize());
}

void HelpViewer::scaleUp()
{
    setZoomFactor(zoomFactor() + 0.1);
}

void HelpViewer::scaleDown()
{
    setZoomFactor(qMax(qreal(0.0), zoomFactor() - qreal(0.1)));
}

void HelpViewer::resetScale()
{
    setZoomFactor(1.0);
}

qreal HelpViewer::scale() const
{
    return zoomFactor();
}

QString HelpViewer::title() const
{
    return QWebView::title();
}

void HelpViewer::setTitle(const QString &title)
{
    Q_UNUSED(title)
}

QUrl HelpViewer::source() const
{
    return url();
}

void HelpViewer::setSource(const QUrl &url)
{
    load(url);
}

QString HelpViewer::selectedText() const
{
    return QWebView::selectedText();
}

bool HelpViewer::isForwardAvailable() const
{
    return pageAction(QWebPage::Forward)->isEnabled();
}

bool HelpViewer::isBackwardAvailable() const
{
    return pageAction(QWebPage::Back)->isEnabled();
}

bool HelpViewer::findText(const QString &text, Find::FindFlags flags,
    bool incremental, bool fromSearch, bool *wrapped)
{
    Q_UNUSED(incremental);
    Q_UNUSED(fromSearch);
    if (wrapped)
        *wrapped = false;
    QWebPage::FindFlags options;
    if (flags & Find::FindBackward)
        options |= QWebPage::FindBackward;
    if (flags & Find::FindCaseSensitively)
        options |= QWebPage::FindCaseSensitively;

    bool found = QWebView::findText(text, options);
    if (!found) {
        options |= QWebPage::FindWrapsAroundDocument;
        found = QWebView::findText(text, options);
        if (found && wrapped)
            *wrapped = true;
    }
    options = QWebPage::HighlightAllOccurrences;
    QWebView::findText(QLatin1String(""), options); // clear first
    QWebView::findText(text, options); // force highlighting of all other matches
    return found;
}

// -- public slots

void HelpViewer::copy()
{
    triggerPageAction(QWebPage::Copy);
}

void HelpViewer::stop()
{
    triggerPageAction(QWebPage::Stop);
}

void HelpViewer::forward()
{
    QWebView::forward();
}

void HelpViewer::backward()
{
    back();
}

// -- protected

void HelpViewer::keyPressEvent(QKeyEvent *e)
{
    // TODO: remove this once we support multiple keysequences per command
    if (e->key() == Qt::Key_Insert && e->modifiers() == Qt::CTRL) {
        if (!selectedText().isEmpty())
            copy();
    }
    QWebView::keyPressEvent(e);
}

void HelpViewer::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers()& Qt::ControlModifier) {
        event->accept();
        event->delta() > 0 ? scaleUp() : scaleDown();
    } else {
        QWebView::wheelEvent(event);
    }
}

void HelpViewer::mousePressEvent(QMouseEvent *event)
{
    if (Utils::HostOsInfo::isLinuxHost() && handleForwardBackwardMouseButtons(event))
        return;

    if (HelpPage *currentPage = static_cast<HelpPage*> (page())) {
        currentPage->m_pressedButtons = event->buttons();
        currentPage->m_keyboardModifiers = event->modifiers();
    }

    QWebView::mousePressEvent(event);
}

void HelpViewer::mouseReleaseEvent(QMouseEvent *event)
{
    if (!Utils::HostOsInfo::isLinuxHost() && handleForwardBackwardMouseButtons(event))
        return;

    QWebView::mouseReleaseEvent(event);
}

// -- private slots

void HelpViewer::actionChanged()
{
    QAction *a = qobject_cast<QAction *>(sender());
    if (a == pageAction(QWebPage::Back))
        emit backwardAvailable(a->isEnabled());
    else if (a == pageAction(QWebPage::Forward))
        emit forwardAvailable(a->isEnabled());
}

void HelpViewer::slotNetworkReplyFinished(QNetworkReply *reply)
{
    if (reply && reply->error() != QNetworkReply::NoError) {
        setSource(QUrl(Help::Constants::AboutBlank));
        setHtml(QString::fromLatin1(g_htmlPage).arg(g_percent1, reply->errorString(),
            HelpViewer::tr("Error loading: %1").arg(reply->url().toString()), g_percent4, g_percent6, g_percent7,
            QString()));
    }
}

// -- private

bool HelpViewer::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        if (QKeyEvent *keyEvent = static_cast<QKeyEvent*> (event)) {
            if (keyEvent->key() == Qt::Key_Slash)
                emit openFindToolBar();
        }
    }
    return QWebView::eventFilter(obj, event);
}

void HelpViewer::contextMenuEvent(QContextMenuEvent *event)
{
    QWebView::contextMenuEvent(event);
}

#endif  // !QT_NO_WEBKIT
