// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "litehtmlhelpviewer.h"

#include "helpconstants.h"
#include "helptr.h"
#include "localhelpmanager.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QScrollBar>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <QDebug>

using namespace Help;
using namespace Help::Internal;

const int kMaxHistoryItems = 20;

static void setLight(QWidget *widget)
{
    QPalette p = widget->palette();
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::Text, Qt::black);
    widget->setPalette(p);
}

static void setPaletteFromTheme(QWidget *widget)
{
    if (Utils::creatorTheme())
        widget->setPalette(Utils::creatorTheme()->palette());
}

static bool isDarkTheme()
{
    return Utils::creatorTheme() && Utils::creatorTheme()->flag(Utils::Theme::DarkUserInterface);
}

static QByteArray getData(const QUrl &url, QWidget *widget)
{
    // This is a hack for Qt documentation,
    // which decides to use a simpler CSS if the viewer does not have JavaScript
    // which was a hack to decide if we are viewing in QTextBrowser or QtWebEngine et al.
    // Force it to use the "normal" offline CSS even without JavaScript, since litehtml can
    // handle that, and inject a dark themed CSS into Qt documentation for dark Qt Creator themes
    QUrl actualUrl = url;
    QString path = url.path(QUrl::FullyEncoded);
    static const char simpleCss[] = "/offline-simple.css";
    if (path.endsWith(simpleCss)) {
        if (isDarkTheme()) {
            // check if dark CSS is shipped with documentation
            QString darkPath = path;
            darkPath.replace(simpleCss, "/offline-dark.css");
            actualUrl.setPath(darkPath);
            LocalHelpManager::HelpData data = LocalHelpManager::helpData(actualUrl);
            if (!data.resolvedUrl.isValid() || data.data.isEmpty()) {
                // fallback
                QFile css(":/help/offline-dark.css");
                if (css.open(QIODevice::ReadOnly))
                    data.data = css.readAll();
            }
            if (!data.data.isEmpty()) {
                // we found the dark style
                // set background dark (by using theme palette)
                setPaletteFromTheme(widget);
                return data.data;
            }
        }
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
    m_viewer->setResourceHandler([this](const QUrl &url) { return getData(url, this); });
    m_viewer->setFrameStyle(QFrame::NoFrame);
    m_viewer->viewport()->installEventFilter(this);
    connect(m_viewer, &QLiteHtmlWidget::linkClicked, this, [this](const QUrl &url) {
        const Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
        if (modifiers == Qt::ControlModifier)
            emit newPageRequested(url);
        else
            setSource(url);
    });
    connect(m_viewer,
            &QLiteHtmlWidget::contextMenuRequested,
            this,
            &LiteHtmlHelpViewer::showContextMenu);
    connect(m_viewer, &QLiteHtmlWidget::linkHighlighted, this, [this](const QUrl &url) {
        m_highlightedLink = url;
        if (!url.isValid())
            QToolTip::hideText();
    });
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

void LiteHtmlHelpViewer::setViewerFont(const QFont &newFont)
{
    m_viewer->setDefaultFont(newFont);
}

void LiteHtmlHelpViewer::setScale(qreal scale)
{
    // interpret 0 as "default"
    m_viewer->setZoomFactor(scale == 0 ? qreal(1) : scale);
}

void LiteHtmlHelpViewer::setAntialias(bool on)
{
    m_viewer->setAntialias(on);
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
    // We control the html, so use theme palette
    setPaletteFromTheme(this);
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
    const QString &text, Utils::FindFlags flags, bool incremental, bool fromSearch, bool *wrapped)
{
    Q_UNUSED(fromSearch)
    return m_viewer->findText(text,
                              Utils::textDocumentFlagsForFindFlags(flags),
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
    const int steps = qMin(count, int(m_forwardItems.size()));
    if (steps == 0)
        return;
    HistoryItem nextItem = currentHistoryItem();
    for (int i = 0; i < steps; ++i) {
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
    const int steps = qMin(count, int(m_backItems.size()));
    if (steps == 0)
        return;
    HistoryItem previousItem = currentHistoryItem();
    for (int i = 0; i < steps; ++i) {
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
    Q_UNUSED(printer)
    // TODO
}

bool LiteHtmlHelpViewer::eventFilter(QObject *src, QEvent *e)
{
    if (isScrollWheelZoomingEnabled() && e->type() == QEvent::Wheel) {
        auto we = static_cast<QWheelEvent *>(e);
        if (we->modifiers() == Qt::ControlModifier) {
            e->ignore();
            return true;
        }
    } else if (e->type() == QEvent::MouseButtonPress) {
        auto me = static_cast<QMouseEvent *>(e);
        if (me->button() == Qt::BackButton) {
            goBackward(1);
            return true;
        } else if (me->button() == Qt::ForwardButton) {
            goForward(1);
            return true;
        }
    } else if (e->type() == QEvent::ToolTip) {
        auto he = static_cast<QHelpEvent *>(e);
        if (m_highlightedLink.isValid())
            QToolTip::showText(he->globalPos(),
                               m_highlightedLink.toDisplayString(),
                               m_viewer->viewport());
    }
    return HelpViewer::eventFilter(src, e);
}

void LiteHtmlHelpViewer::setSourceInternal(const QUrl &url, std::optional<int> vscroll)
{
    slotLoadStarted();
    QUrl currentUrlWithoutFragment = m_viewer->url();
    currentUrlWithoutFragment.setFragment({});
    QUrl newUrlWithoutFragment = url;
    newUrlWithoutFragment.setFragment({});
    m_viewer->setUrl(url);
    if (currentUrlWithoutFragment != newUrlWithoutFragment) {
        // We do not expect the documentation to support dark themes, so start with light palette.
        // We override this if we find Qt's dark style
        setLight(this);
        m_viewer->setHtml(QString::fromUtf8(getData(url, this)));
    }
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
            QAction *action = menu.addAction(Tr::tr(Constants::TR_OPEN_LINK_AS_NEW_PAGE));
            connect(action, &QAction::triggered, this, [this, url]() {
                emit newPageRequested(url);
            });
        }
        if (isActionVisible(HelpViewer::Action::ExternalWindow)) {
            QAction *action = menu.addAction(Tr::tr(Constants::TR_OPEN_LINK_IN_WINDOW));
            connect(action, &QAction::triggered, this, [this, url]() {
                emit externalPageRequested(url);
            });
        }
        copyAnchorAction = menu.addAction(Tr::tr("Copy Link"));
    } else if (!m_viewer->selectedText().isEmpty()) {
        connect(menu.addAction(Tr::tr("Copy")), &QAction::triggered, this, &HelpViewer::copy);
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
