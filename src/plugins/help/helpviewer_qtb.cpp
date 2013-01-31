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

#if defined(QT_NO_WEBKIT)

#include "helpconstants.h"
#include "helpviewer_p.h"
#include "localhelpmanager.h"

#include <utils/hostosinfo.h>

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>

#include <QHelpEngine>

using namespace Find;
using namespace Help;
using namespace Help::Internal;

// -- HelpViewer

HelpViewer::HelpViewer(qreal zoom, QWidget *parent)
    : QTextBrowser(parent)
    , d(new HelpViewerPrivate(zoom))
{
    QPalette p = palette();
    p.setColor(QPalette::Inactive, QPalette::Highlight,
        p.color(QPalette::Active, QPalette::Highlight));
    p.setColor(QPalette::Inactive, QPalette::HighlightedText,
        p.color(QPalette::Active, QPalette::HighlightedText));
    setPalette(p);

    installEventFilter(this);
    document()->setDocumentMargin(8);

    QFont font = viewerFont();
    font.setPointSize(int(font.pointSize() + zoom));
    setViewerFont(font);

    connect(this, SIGNAL(sourceChanged(QUrl)), this, SIGNAL(titleChanged()));
    connect(this, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished(bool)));
}

HelpViewer::~HelpViewer()
{
    delete d;
}

QFont HelpViewer::viewerFont() const
{
    const QHelpEngineCore &engine = LocalHelpManager::helpEngine();
    return qvariant_cast<QFont>(engine.customValue(QLatin1String("font"),
        qApp->font()));
}

void HelpViewer::setViewerFont(const QFont &newFont)
{
    if (font() != newFont) {
        d->forceFont = true;
        setFont(newFont);
        d->forceFont = false;
    }
}

void HelpViewer::scaleUp()
{
    if (d->zoomCount < 10) {
        d->zoomCount++;
        d->forceFont = true;
        zoomIn();
        d->forceFont = false;
    }
}

void HelpViewer::scaleDown()
{
    if (d->zoomCount > -5) {
        d->zoomCount--;
        d->forceFont = true;
        zoomOut();
        d->forceFont = false;
    }
}

void HelpViewer::resetScale()
{
    if (d->zoomCount != 0) {
        d->forceFont = true;
        zoomOut(d->zoomCount);
        d->forceFont = false;
    }
    d->zoomCount = 0;
}

qreal HelpViewer::scale() const
{
    return d->zoomCount;
}

QString HelpViewer::title() const
{
    return documentTitle();
}

void HelpViewer::setTitle(const QString &title)
{
    setDocumentTitle(title);
}

QUrl HelpViewer::source() const
{
    return QTextBrowser::source();
}

void HelpViewer::setSource(const QUrl &url)
{
    const QString &string = url.toString();
    if (url.isValid() && string != QLatin1String("help")) {
        if (launchWithExternalApp(url))
            return;

        QUrl resolvedUrl;
        if (url.scheme() == QLatin1String("http"))
            resolvedUrl = url;

        if (!resolvedUrl.isValid()) {
            const QHelpEngineCore &engine = LocalHelpManager::helpEngine();
            resolvedUrl = engine.findFile(url);
        }

        if (resolvedUrl.isValid()) {
            QTextBrowser::setSource(resolvedUrl);
            emit loadFinished(true);
            return;
        }
    }

    QTextBrowser::setSource(url);
    setHtml(string == Help::Constants::AboutBlank
        ? HelpViewer::tr("<title>about:blank</title>")
        : HelpViewer::tr("<html><head><meta http-equiv=\""
            "content-type\" content=\"text/html; charset=UTF-8\"><title>Error 404...</title>"
            "</head><body><div align=\"center\"><br><br><h1>The page could not be found</h1>"
            "<br><h3>'%1'</h3></div></body>")
          .arg(url.toString()));

    emit loadFinished(true);
}

QString HelpViewer::selectedText() const
{
    return textCursor().selectedText();
}

bool HelpViewer::isForwardAvailable() const
{
    return QTextBrowser::isForwardAvailable();
}

bool HelpViewer::isBackwardAvailable() const
{
    return QTextBrowser::isBackwardAvailable();
}

bool HelpViewer::findText(const QString &text, Find::FindFlags flags,
    bool incremental, bool fromSearch, bool *wrapped)
{
    if (wrapped)
        *wrapped = false;
    QTextDocument *doc = document();
    QTextCursor cursor = textCursor();
    if (!doc || cursor.isNull())
        return false;

    const int position = cursor.selectionStart();
    if (incremental)
        cursor.setPosition(position);

    QTextDocument::FindFlags f = Find::textDocumentFlagsForFindFlags(flags);
    QTextCursor found = doc->find(text, cursor, f);
    if (found.isNull()) {
        if ((flags & Find::FindBackward) == 0)
            cursor.movePosition(QTextCursor::Start);
        else
            cursor.movePosition(QTextCursor::End);
        found = doc->find(text, cursor, f);
        if (!found.isNull() && wrapped)
            *wrapped = true;
    }

    if (fromSearch) {
        cursor.beginEditBlock();
        viewport()->setUpdatesEnabled(false);

        QTextCharFormat marker;
        marker.setForeground(Qt::red);
        cursor.movePosition(QTextCursor::Start);
        setTextCursor(cursor);

        while (find(text)) {
            QTextCursor hit = textCursor();
            hit.mergeCharFormat(marker);
        }

        viewport()->setUpdatesEnabled(true);
        cursor.endEditBlock();
    }

    bool cursorIsNull = found.isNull();
    if (cursorIsNull) {
        found = textCursor();
        found.setPosition(position);
    }
    setTextCursor(found);
    return !cursorIsNull;
}

// -- public slots

void HelpViewer::copy()
{
    QTextBrowser::copy();
}

void HelpViewer::stop()
{
}

void HelpViewer::forward()
{
    QTextBrowser::forward();
}

void HelpViewer::backward()
{
    QTextBrowser::backward();
}

// -- protected

void HelpViewer::keyPressEvent(QKeyEvent *e)
{
    if ((e->key() == Qt::Key_Home && e->modifiers() != Qt::NoModifier)
        || (e->key() == Qt::Key_End && e->modifiers() != Qt::NoModifier)) {
        QKeyEvent* event = new QKeyEvent(e->type(), e->key(), Qt::NoModifier,
            e->text(), e->isAutoRepeat(), e->count());
        e = event;
    }
    QTextBrowser::keyPressEvent(e);
}

void HelpViewer::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() == Qt::ControlModifier) {
        e->accept();
        e->delta() > 0 ? scaleUp() : scaleDown();
    } else {
        QTextBrowser::wheelEvent(e);
    }
}

void HelpViewer::mousePressEvent(QMouseEvent *e)
{
    if (Utils::HostOsInfo::isLinuxHost() && handleForwardBackwardMouseButtons(e))
        return;
    QTextBrowser::mousePressEvent(e);
}

void HelpViewer::mouseReleaseEvent(QMouseEvent *e)
{
    if (!Utils::HostOsInfo::isLinuxHost() && handleForwardBackwardMouseButtons(e))
        return;

    bool controlPressed = e->modifiers() & Qt::ControlModifier;
    if ((controlPressed && d->hasAnchorAt(this, e->pos())) ||
        (e->button() == Qt::MidButton && d->hasAnchorAt(this, e->pos()))) {
        d->openLinkInNewPage();
        return;
    }

    QTextBrowser::mouseReleaseEvent(e);
}

// -- private slots

void HelpViewer::actionChanged()
{
    // stub
}

// -- private

bool HelpViewer::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FontChange && !d->forceFont)
        return true;

    if (event->type() == QEvent::KeyPress) {
        if (QKeyEvent *keyEvent = static_cast<QKeyEvent*> (event)) {
            if (keyEvent->key() == Qt::Key_Slash)
                emit openFindToolBar();
        }
    }
    return QTextBrowser::eventFilter(obj, event);
}

void HelpViewer::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(QLatin1String(""), 0);

    QUrl link;
    QAction *copyAnchorAction = 0;
    if (d->hasAnchorAt(this, event->pos())) {
        link = anchorAt(event->pos());
        if (link.isRelative())
            link = source().resolved(link);
        menu.addAction(tr("Open Link"), d, SLOT(openLink()));
        menu.addAction(tr("Open Link as New Page"), d, SLOT(openLinkInNewPage()));

        if (!link.isEmpty() && link.isValid())
            copyAnchorAction = menu.addAction(tr("Copy Link"));
    } else if (!selectedText().isEmpty()) {
        menu.addAction(tr("Copy"), this, SLOT(copy()));
    } else {
        menu.addAction(tr("Reload"), this, SLOT(reload()));
    }

    if (copyAnchorAction == menu.exec(event->globalPos()))
        QApplication::clipboard()->setText(link.toString());
}

QVariant HelpViewer::loadResource(int type, const QUrl &name)
{
    QByteArray ba;
    if (type < 4) {
        const QHelpEngineCore &engine = LocalHelpManager::helpEngine();
        ba = engine.fileData(name);
        if (name.toString().endsWith(QLatin1String(".svg"), Qt::CaseInsensitive)) {
            QImage image;
            image.loadFromData(ba, "svg");
            if (!image.isNull())
                return image;
        }
    }
    return ba;
}

#endif  // QT_NO_WEBKIT
