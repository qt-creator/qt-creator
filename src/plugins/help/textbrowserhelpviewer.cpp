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

#include "textbrowserhelpviewer.h"

#include "helpconstants.h"
#include "localhelpmanager.h"

#include <coreplugin/find/findplugin.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QScrollBar>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>

using namespace Help;
using namespace Help::Internal;

// -- HelpViewer

TextBrowserHelpViewer::TextBrowserHelpViewer(QWidget *parent)
    : HelpViewer(parent)
    , m_textBrowser(new TextBrowserHelpWidget(this))
{
    m_textBrowser->setOpenLinks(false);
    auto layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_textBrowser, 10);
    setFocusProxy(m_textBrowser);
    QPalette p = palette();
    p.setColor(QPalette::Inactive, QPalette::Highlight,
        p.color(QPalette::Active, QPalette::Highlight));
    p.setColor(QPalette::Inactive, QPalette::HighlightedText,
        p.color(QPalette::Active, QPalette::HighlightedText));
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::Text, Qt::black);
    setPalette(p);

    connect(m_textBrowser, &TextBrowserHelpWidget::anchorClicked,
            this, &TextBrowserHelpViewer::setSource);
    connect(m_textBrowser, &QTextBrowser::sourceChanged, this, &HelpViewer::titleChanged);
    connect(m_textBrowser, &QTextBrowser::forwardAvailable, this, &HelpViewer::forwardAvailable);
    connect(m_textBrowser, &QTextBrowser::backwardAvailable, this, &HelpViewer::backwardAvailable);
}

TextBrowserHelpViewer::~TextBrowserHelpViewer() = default;

void TextBrowserHelpViewer::setViewerFont(const QFont &newFont)
{
    setFontAndScale(newFont, LocalHelpManager::fontZoom() / 100.0);
}

void TextBrowserHelpViewer::setFontAndScale(const QFont &font, qreal scale)
{
    m_textBrowser->withFixedTopPosition([this, &font, scale] {
        QFont newFont = font;
        const float newSize = font.pointSizeF() * scale;
        newFont.setPointSizeF(newSize);
        m_textBrowser->setFont(newFont);
    });
}

void TextBrowserHelpViewer::setScale(qreal scale)
{
    setFontAndScale(LocalHelpManager::fallbackFont(), scale);
}

QString TextBrowserHelpViewer::title() const
{
    return m_textBrowser->documentTitle();
}

QUrl TextBrowserHelpViewer::source() const
{
    return m_textBrowser->source();
}

void TextBrowserHelpViewer::setSource(const QUrl &url)
{
    if (launchWithExternalApp(url))
        return;

    slotLoadStarted();
    m_textBrowser->setSource(url);
    if (!url.fragment().isEmpty())
        m_textBrowser->scrollToAnchor(url.fragment());
    if (QScrollBar *hScrollBar = m_textBrowser->horizontalScrollBar())
        hScrollBar->setValue(0);
    slotLoadFinished();
}

void TextBrowserHelpViewer::setHtml(const QString &html)
{
    m_textBrowser->setHtml(html);
}

QString TextBrowserHelpViewer::selectedText() const
{
    return m_textBrowser->textCursor().selectedText();
}

bool TextBrowserHelpViewer::isForwardAvailable() const
{
    return m_textBrowser->isForwardAvailable();
}

bool TextBrowserHelpViewer::isBackwardAvailable() const
{
    return m_textBrowser->isBackwardAvailable();
}

void TextBrowserHelpViewer::addBackHistoryItems(QMenu *backMenu)
{
    for (int i = 1; i <= m_textBrowser->backwardHistoryCount(); ++i) {
        auto action = new QAction(backMenu);
        action->setText(m_textBrowser->historyTitle(-i));
        action->setData(-i);
        connect(action, &QAction::triggered, this, &TextBrowserHelpViewer::goToHistoryItem);
        backMenu->addAction(action);
    }
}

void TextBrowserHelpViewer::addForwardHistoryItems(QMenu *forwardMenu)
{
    for (int i = 1; i <= m_textBrowser->forwardHistoryCount(); ++i) {
        auto action = new QAction(forwardMenu);
        action->setText(m_textBrowser->historyTitle(i));
        action->setData(i);
        connect(action, &QAction::triggered, this, &TextBrowserHelpViewer::goToHistoryItem);
        forwardMenu->addAction(action);
    }
}

bool TextBrowserHelpViewer::findText(const QString &text, Core::FindFlags flags,
    bool incremental, bool fromSearch, bool *wrapped)
{
    if (wrapped)
        *wrapped = false;
    QTextDocument *doc = m_textBrowser->document();
    QTextCursor cursor = m_textBrowser->textCursor();
    if (!doc || cursor.isNull())
        return false;

    const int position = cursor.selectionStart();
    if (incremental)
        cursor.setPosition(position);

    QTextDocument::FindFlags f = Core::textDocumentFlagsForFindFlags(flags);
    QTextCursor found = doc->find(text, cursor, f);
    if (found.isNull()) {
        if ((flags & Core::FindBackward) == 0)
            cursor.movePosition(QTextCursor::Start);
        else
            cursor.movePosition(QTextCursor::End);
        found = doc->find(text, cursor, f);
        if (!found.isNull() && wrapped)
            *wrapped = true;
    }

    if (fromSearch) {
        cursor.beginEditBlock();
        m_textBrowser->viewport()->setUpdatesEnabled(false);

        QTextCharFormat marker;
        marker.setForeground(Qt::red);
        cursor.movePosition(QTextCursor::Start);
        m_textBrowser->setTextCursor(cursor);

        while (m_textBrowser->find(text)) {
            QTextCursor hit = m_textBrowser->textCursor();
            hit.mergeCharFormat(marker);
        }

        m_textBrowser->viewport()->setUpdatesEnabled(true);
        cursor.endEditBlock();
    }

    bool cursorIsNull = found.isNull();
    if (cursorIsNull) {
        found = m_textBrowser->textCursor();
        found.setPosition(position);
    }
    m_textBrowser->setTextCursor(found);
    return !cursorIsNull;
}

void TextBrowserHelpViewer::copy()
{
    m_textBrowser->copy();
}

void TextBrowserHelpViewer::stop()
{
}

void TextBrowserHelpViewer::forward()
{
    slotLoadStarted();
    m_textBrowser->forward();
    slotLoadFinished();
}

void TextBrowserHelpViewer::backward()
{
    slotLoadStarted();
    m_textBrowser->backward();
    slotLoadFinished();
}

void TextBrowserHelpViewer::print(QPrinter *printer)
{
    m_textBrowser->print(printer);
}

void TextBrowserHelpViewer::goToHistoryItem()
{
    auto action = qobject_cast<const QAction *>(sender());
    QTC_ASSERT(action, return);
    bool ok = false;
    int index = action->data().toInt(&ok);
    QTC_ASSERT(ok, return);
    // go back?
    while (index < 0) {
        m_textBrowser->backward();
        ++index;
    }
    // go forward?
    while (index > 0) {
        m_textBrowser->forward();
        --index;
    }
}

// -- private

TextBrowserHelpWidget::TextBrowserHelpWidget(TextBrowserHelpViewer *parent)
    : QTextBrowser(parent)
    , m_parent(parent)
{
    setFrameShape(QFrame::NoFrame);
    installEventFilter(this);
    document()->setDocumentMargin(8);
}

QVariant TextBrowserHelpWidget::loadResource(int type, const QUrl &name)
{
    if (type < QTextDocument::UserResource)
        return LocalHelpManager::helpData(name).data;
    return QByteArray();
}

QString TextBrowserHelpWidget::linkAt(const QPoint &pos)
{
    QString anchor = anchorAt(pos);
    if (anchor.isEmpty())
        return QString();

    anchor = source().resolved(anchor).toString();
    if (anchor.at(0) == QLatin1Char('#')) {
        QString src = source().toString();
        int hsh = src.indexOf(QLatin1Char('#'));
        anchor = (hsh >= 0 ? src.left(hsh) : src) + anchor;
    }
    return anchor;
}

void TextBrowserHelpWidget::withFixedTopPosition(const std::function<void()> &action)
{
    const int topTextPosition = cursorForPosition({width() / 2, 0}).position();
    action();
    scrollToTextPosition(topTextPosition);
}

void TextBrowserHelpWidget::scrollToTextPosition(int position)
{
    QTextCursor tc(document());
    tc.setPosition(position);
    const int dy = cursorRect(tc).top();
    if (verticalScrollBar()) {
        verticalScrollBar()->setValue(
            std::min(verticalScrollBar()->value() + dy, verticalScrollBar()->maximum()));
    }
}

void TextBrowserHelpWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu("", nullptr);

    QAction *copyAnchorAction = nullptr;
    const QUrl link(linkAt(event->pos()));
    if (!link.isEmpty() && link.isValid()) {
        QAction *action = menu.addAction(tr("Open Link"));
        connect(action, &QAction::triggered, this, [this, link]() {
            setSource(link);
        });
        if (m_parent->isActionVisible(HelpViewer::Action::NewPage)) {
            action = menu.addAction(QCoreApplication::translate("HelpViewer", Constants::TR_OPEN_LINK_AS_NEW_PAGE));
            connect(action, &QAction::triggered, this, [this, link]() {
                emit m_parent->newPageRequested(link);
            });
        }
        if (m_parent->isActionVisible(HelpViewer::Action::ExternalWindow)) {
            action = menu.addAction(QCoreApplication::translate("HelpViewer", Constants::TR_OPEN_LINK_IN_WINDOW));
            connect(action, &QAction::triggered, this, [this, link]() {
                emit m_parent->externalPageRequested(link);
            });
        }
        copyAnchorAction = menu.addAction(tr("Copy Link"));
    } else if (!textCursor().selectedText().isEmpty()) {
        connect(menu.addAction(tr("Copy")), &QAction::triggered, this, &QTextEdit::copy);
    }

    if (copyAnchorAction == menu.exec(event->globalPos()))
        QApplication::clipboard()->setText(link.toString());
}

bool TextBrowserHelpWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == this) {
        if (event->type() == QEvent::KeyPress) {
            auto keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Slash) {
                keyEvent->accept();
                Core::Find::openFindToolBar(Core::Find::FindForwardDirection);
                return true;
            }
        } else if (event->type() == QEvent::ToolTip) {
            auto e = static_cast<const QHelpEvent *>(event);
            QToolTip::showText(e->globalPos(), linkAt(e->pos()));
            return true;
        }
    }
    return QTextBrowser::eventFilter(obj, event);
}

void TextBrowserHelpWidget::wheelEvent(QWheelEvent *e)
{
    // These two conditions should match those defined in QTextEdit::wheelEvent()
    if (!(textInteractionFlags() & Qt::TextEditable)) {
        if (e->modifiers() & Qt::ControlModifier) {
            // Don't handle wheelEvent by the QTextEdit superclass, which zooms the
            // view in a broken way. We handle it properly through the sequence:
            // HelpViewer::wheelEvent() -> LocalHelpManager::setFontZoom() ->
            // HelpViewer::setFontZoom() -> TextBrowserHelpViewer::setFontAndScale().
            e->ignore();
            return;
        }
    }
    QTextBrowser::wheelEvent(e);
}

void TextBrowserHelpWidget::mousePressEvent(QMouseEvent *e)
{
    if (Utils::HostOsInfo::isLinuxHost() && m_parent->handleForwardBackwardMouseButtons(e))
        return;
    QTextBrowser::mousePressEvent(e);
}

void TextBrowserHelpWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (!Utils::HostOsInfo::isLinuxHost() && m_parent->handleForwardBackwardMouseButtons(e))
        return;

    bool controlPressed = e->modifiers() & Qt::ControlModifier;
    const QString link = linkAt(e->pos());
    if (m_parent->isActionVisible(HelpViewer::Action::NewPage)
            && (controlPressed || e->button() == Qt::MiddleButton) && !link.isEmpty()) {
        emit m_parent->newPageRequested(QUrl(link));
        return;
    }

    QTextBrowser::mouseReleaseEvent(e);
}

void TextBrowserHelpWidget::resizeEvent(QResizeEvent *e)
{
    const int topTextPosition = cursorForPosition({width() / 2, 0}).position();
    QTextBrowser::resizeEvent(e);
    scrollToTextPosition(topTextPosition);
}
