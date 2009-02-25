/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "centralwidget.h"
#include "helpviewer.h"
#include "topicchooser.h"

#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QTimer>

#include <QtGui/QMenu>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QPrinter>
#include <QtGui/QLineEdit>
#include <QtGui/QCheckBox>
#include <QtGui/QTabBar>
#include <QtGui/QTabWidget>
#include <QtGui/QToolButton>
#include <QtGui/QMouseEvent>
#include <QtGui/QFocusEvent>
#include <QtGui/QMainWindow>
#include <QtGui/QSpacerItem>
#include <QtGui/QTextCursor>
#include <QtGui/QPrintDialog>
#include <QtGui/QApplication>
#include <QtGui/QTextDocumentFragment>
#include <QtGui/QPrintPreviewDialog>
#include <QtGui/QPageSetupDialog>

#include <QtHelp/QHelpEngine>

#include <coreplugin/coreconstants.h>

using namespace Help::Internal;

namespace {
    HelpViewer* helpViewerFromTabPosition(const QTabWidget *widget, const QPoint &point)
    {
        QTabBar *tabBar = qFindChild<QTabBar*>(widget);
        for (int i = 0; i < tabBar->count(); ++i) {
            if (tabBar->tabRect(i).contains(point))
                return qobject_cast<HelpViewer*>(widget->widget(i));
        }
        return 0;
    }
    CentralWidget *staticCentralWidget = 0;
}

CentralWidget::CentralWidget(QHelpEngine *engine, QWidget *parent)
    : QWidget(parent)
    , findBar(0)
    , tabWidget(0)
    , helpEngine(engine)
    , printer(0)
{
    lastTabPage = 0;
    globalActionList.clear();
    collectionFile = helpEngine->collectionFile();

    QString system = QLatin1String("win");

#ifdef Q_OS_MAC
    system = QLatin1String("mac");
#endif

    tabWidget = new QTabWidget;
    tabWidget->setDocumentMode(true);
    tabWidget->setMovable(true);
    connect(tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentPageChanged(int)));

    QToolButton *newTabButton = new QToolButton(this);
    newTabButton->setAutoRaise(true);
    newTabButton->setToolTip(tr("Add new page"));
    newTabButton->setIcon(QIcon(QString::fromUtf8(":/trolltech/assistant/images/%1/addtab.png").arg(system)));

    tabWidget->setCornerWidget(newTabButton, Qt::TopLeftCorner);
    connect(newTabButton, SIGNAL(clicked()), this, SLOT(newTab()));

    QVBoxLayout *vboxLayout = new QVBoxLayout(this);
    vboxLayout->setMargin(0);
    vboxLayout->addWidget(tabWidget);

    QTabBar *tabBar = qFindChild<QTabBar*>(tabWidget);
    if (tabBar) {
        tabBar->installEventFilter(this);
        tabBar->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(tabBar, SIGNAL(customContextMenuRequested(const QPoint&)),
                this, SLOT(showTabBarContextMenu(const QPoint&)));
    }

    staticCentralWidget = this;
}

CentralWidget::~CentralWidget()
{
    QDir dir;
    QString currentPages;
    QHelpEngineCore engine(collectionFile, 0);

    if (engine.setupData()) {
        for (int i = 0; i < tabWidget->count(); ++i) {
            HelpViewer *viewer = qobject_cast<HelpViewer*>(tabWidget->widget(i));
            if (viewer && viewer->source().isValid())
                currentPages.append(viewer->source().toString()).append(QLatin1Char('|'));
        }
        engine.setCustomValue(QLatin1String("LastTabPage"), lastTabPage);
        engine.setCustomValue(QLatin1String("LastShownPages"), currentPages);
    }
}

CentralWidget *CentralWidget::instance()
{
    return staticCentralWidget;
}

void CentralWidget::newTab()
{
    HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        setSourceInNewTab(viewer->source());
}

void CentralWidget::zoomIn()
{
    HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        viewer->zoomIn();
}

void CentralWidget::zoomOut()
{
    HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        viewer->zoomOut();
}

void CentralWidget::nextPage()
{
    if (tabWidget->currentIndex() < tabWidget->count() -1)
        tabWidget->setCurrentIndex(tabWidget->currentIndex() +1);
    else
        tabWidget->setCurrentIndex(0);
}

void CentralWidget::resetZoom()
{
    HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        viewer->resetZoom();
}

void CentralWidget::previousPage()
{
    int index = tabWidget->currentIndex() -1;
    if (index >= 0)
        tabWidget->setCurrentIndex(index);
    else
        tabWidget->setCurrentIndex(tabWidget->count() -1);
}

void CentralWidget::closeTab()
{
    closeTab(tabWidget->currentIndex());
}

void CentralWidget::closeTab(int index)
{
    HelpViewer* viewer = helpViewerAtIndex(index);
    if (!viewer || tabWidget->count() == 1)
        return;

    tabWidget->removeTab(index);
    QTimer::singleShot(0, viewer, SLOT(deleteLater()));
}

void CentralWidget::setSource(const QUrl &url)
{
    HelpViewer* viewer = currentHelpViewer();
    HelpViewer* lastViewer = qobject_cast<HelpViewer*>(tabWidget->widget(lastTabPage));

    if (!viewer && !lastViewer) {
        viewer = new HelpViewer(helpEngine, this);
        viewer->installEventFilter(this);
        lastTabPage = tabWidget->addTab(viewer, QString());
        tabWidget->setCurrentIndex(lastTabPage);
        connectSignals();
        qApp->processEvents();
    } else
        viewer = lastViewer;

    viewer->setSource(url);
    currentPageChanged(lastTabPage);
    viewer->setFocus(Qt::OtherFocusReason);
    tabWidget->setCurrentIndex(lastTabPage);
    tabWidget->setTabText(lastTabPage, quoteTabTitle(viewer->documentTitle()));
}

void CentralWidget::setLastShownPages()
{
    const QStringList lastShownPageList =
        helpEngine->customValue(QLatin1String("LastShownPages")). toString().
        split(QLatin1Char('|'), QString::SkipEmptyParts);

    if (!lastShownPageList.isEmpty()) {
        foreach (const QString& page, lastShownPageList)
            setSourceInNewTab(page);

        tabWidget->setCurrentIndex(helpEngine->
            customValue(QLatin1String("LastTabPage"), 0).toInt());
    } else {
        QUrl url(helpEngine->findFile(QUrl("qthelp://com.trolltech.qt.440/qdoc/index.html")));
        if (!url.isValid()) {
            url.setUrl(QString("qthelp://com.nokia.qtcreator.%1%2/doc/index.html").
                arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR));
        }
        setSource(url);
    }
}

bool CentralWidget::hasSelection() const
{
    const HelpViewer* viewer = currentHelpViewer();
    return viewer ? viewer->hasSelection() : false;
}

QUrl CentralWidget::currentSource() const
{
    const HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        return viewer->source();

    return QUrl();
}

QString CentralWidget::currentTitle() const
{
    const HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        return viewer->documentTitle();

    return QString();
}

void CentralWidget::copySelection()
{
    HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        viewer->copy();
}

void CentralWidget::initPrinter()
{
#ifndef QT_NO_PRINTER
    if (!printer)
        printer = new QPrinter(QPrinter::HighResolution);
#endif
}

void CentralWidget::print()
{
#ifndef QT_NO_PRINTER
    HelpViewer* viewer = currentHelpViewer();
    if (!viewer)
        return;

    initPrinter();

    QPrintDialog *dlg = new QPrintDialog(printer, this);
#if defined(QT_NO_WEBKIT)
    if (viewer->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
#endif
    dlg->addEnabledOption(QAbstractPrintDialog::PrintPageRange);
    dlg->addEnabledOption(QAbstractPrintDialog::PrintCollateCopies);
    dlg->setWindowTitle(tr("Print Document"));
    if (dlg->exec() == QDialog::Accepted) {
        viewer->print(printer);
    }
    delete dlg;
#endif
}

void CentralWidget::printPreview()
{
#ifndef QT_NO_PRINTER
    initPrinter();
    QPrintPreviewDialog preview(printer, this);
    connect(&preview, SIGNAL(paintRequested(QPrinter *)), SLOT(printPreview(QPrinter *)));
    preview.exec();
#endif
}

void CentralWidget::printPreview(QPrinter *p)
{
#ifndef QT_NO_PRINTER
    HelpViewer *viewer = currentHelpViewer();
    if (viewer)
        viewer->print(p);
#endif
}

void CentralWidget::pageSetup()
{
#ifndef QT_NO_PRINTER
    initPrinter();
    QPageSetupDialog dlg(printer);
    dlg.exec();
#endif
}

bool CentralWidget::isHomeAvailable() const
{
    return currentHelpViewer() ? true : false;
}

void CentralWidget::home()
{
    HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        viewer->home();
}

bool CentralWidget::isForwardAvailable() const
{
    const HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        return viewer->isForwardAvailable();

    return false;
}

void CentralWidget::forward()
{
    HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        viewer->forward();
}

bool CentralWidget::isBackwardAvailable() const
{
    const HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        return viewer->isBackwardAvailable();

    return false;
}

void CentralWidget::backward()
{
    HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        viewer->backward();
}


QList<QAction*> CentralWidget::globalActions() const
{
    return globalActionList;
}

void CentralWidget::setGlobalActions(const QList<QAction*> &actions)
{
    globalActionList = actions;
}

void CentralWidget::setSourceInNewTab(const QUrl &url)
{
    HelpViewer* viewer = new HelpViewer(helpEngine, this);
    viewer->installEventFilter(this);
    viewer->setSource(url);
    viewer->setFocus(Qt::OtherFocusReason);
    tabWidget->setCurrentIndex(tabWidget->addTab(viewer,
        quoteTabTitle(viewer->documentTitle())));

    connectSignals();
}

HelpViewer *CentralWidget::newEmptyTab()
{
    HelpViewer* viewer = new HelpViewer(helpEngine, this);
    viewer->installEventFilter(this);
    viewer->setFocus(Qt::OtherFocusReason);
#if defined(QT_NO_WEBKIT)
    viewer->setDocumentTitle(tr("unknown"));
#endif
    tabWidget->setCurrentIndex(tabWidget->addTab(viewer, tr("unknown")));

    connectSignals();
    return viewer;
}

void CentralWidget::connectSignals()
{
    const HelpViewer* viewer = currentHelpViewer();
    if (viewer) {
        connect(viewer, SIGNAL(copyAvailable(bool)), this, SIGNAL(copyAvailable(bool)));
        connect(viewer, SIGNAL(forwardAvailable(bool)), this, SIGNAL(forwardAvailable(bool)));
        connect(viewer, SIGNAL(backwardAvailable(bool)), this, SIGNAL(backwardAvailable(bool)));
        connect(viewer, SIGNAL(sourceChanged(const QUrl&)), this, SIGNAL(sourceChanged(const QUrl&)));
        connect(viewer, SIGNAL(highlighted(const QString&)), this, SIGNAL(highlighted(const QString&)));

        connect(viewer, SIGNAL(sourceChanged(const QUrl&)), this, SLOT(setTabTitle(const QUrl&)));
    }
}

HelpViewer *CentralWidget::helpViewerAtIndex(int index) const
{
    return qobject_cast<HelpViewer*>(tabWidget->widget(index));
}

HelpViewer *CentralWidget::currentHelpViewer() const
{
    return qobject_cast<HelpViewer*>(tabWidget->currentWidget());
}

void CentralWidget::activateTab(bool onlyHelpViewer)
{
    if (currentHelpViewer()) {
        currentHelpViewer()->setFocus();
    } else {
        int idx = 0;
        if (onlyHelpViewer)
            idx = lastTabPage;
        tabWidget->setCurrentIndex(idx);
        tabWidget->currentWidget()->setFocus();
    }
}

void CentralWidget::setTabTitle(const QUrl& url)
{
    int tab = lastTabPage;
    HelpViewer* viewer = currentHelpViewer();

#if !defined(QT_NO_WEBKIT)
    if (!viewer || viewer->source() != url) {
        QTabBar *tabBar = qFindChild<QTabBar*>(tabWidget);
        for (tab = 0; tab < tabBar->count(); ++tab) {
            viewer = qobject_cast<HelpViewer*>(tabWidget->widget(tab));
            if (viewer && viewer->source() == url)
                break;
        }
    }
#endif

    if (viewer) {
        tabWidget->setTabText(tab,
            quoteTabTitle(viewer->documentTitle().trimmed()));
    }
}

void CentralWidget::currentPageChanged(int index)
{
    const HelpViewer *viewer = currentHelpViewer();

    if (viewer || tabWidget->count() == 1)
        lastTabPage = index;

    bool enabled = false;
    if (viewer)
        enabled = tabWidget->count() > 1;

    tabWidget->setTabsClosable(enabled);
    tabWidget->cornerWidget(Qt::TopLeftCorner)->setEnabled(true);

     emit currentViewerChanged();
}

void CentralWidget::showTabBarContextMenu(const QPoint &point)
{
    HelpViewer* viewer = helpViewerFromTabPosition(tabWidget, point);
    if (!viewer)
        return;

    QTabBar *tabBar = qFindChild<QTabBar*>(tabWidget);

    QMenu menu(QLatin1String(""), tabBar);
    QAction *new_page = menu.addAction(tr("Add New Page"));
    QAction *close_page = menu.addAction(tr("Close This Page"));
    QAction *close_pages = menu.addAction(tr("Close Other Pages"));
    menu.addSeparator();
    QAction *newBookmark = menu.addAction(tr("Add Bookmark for this Page..."));

    if (tabBar->count() == 1) {
        close_page->setEnabled(false);
        close_pages->setEnabled(false);
    }

    QAction *picked_action = menu.exec(tabBar->mapToGlobal(point));
    if (!picked_action)
        return;

    if (picked_action == new_page)
        setSourceInNewTab(viewer->source());

    if (picked_action == close_page) {
        tabWidget->removeTab(tabWidget->indexOf(viewer));
        QTimer::singleShot(0, viewer, SLOT(deleteLater()));
    }

    if (picked_action == close_pages) {
        int currentPage = tabWidget->indexOf(viewer);
        for (int i = tabBar->count() -1; i >= 0; --i) {
            viewer = qobject_cast<HelpViewer*>(tabWidget->widget(i));
            if (i != currentPage && viewer) {
                tabWidget->removeTab(i);
                QTimer::singleShot(0, viewer, SLOT(deleteLater()));

                if (i < currentPage)
                    --currentPage;
            }
        }
    }

    if (picked_action == newBookmark)
        emit addNewBookmark(viewer->documentTitle(), viewer->source().toString());
}

// if we have a current help viewer then this is the 'focus proxy', otherwise
// it's the tab widget itself
// this is needed, so an embedding program can just set the focus to the central widget
// and it does TheRightThing
void CentralWidget::focusInEvent(QFocusEvent * /* event */)
{
    if (currentHelpViewer())
        QTimer::singleShot(1, currentHelpViewer(), SLOT(setFocus()));
    else
        QTimer::singleShot(1, tabWidget, SLOT(setFocus()));
}

bool CentralWidget::eventFilter(QObject *object, QEvent *e)
{
    if (currentHelpViewer() == object && e->type() == QEvent::KeyPress){
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);
        if (ke->key() == Qt::Key_Backspace) {
            HelpViewer *viewer = currentHelpViewer();
            if (viewer && viewer->isBackwardAvailable())
                viewer->backward();
            return true;
        }
    }

    QTabBar *tabBar = qobject_cast<QTabBar*>(object);
    bool mousRel = e->type() == QEvent::MouseButtonRelease;
    bool dblClick = e->type() == QEvent::MouseButtonDblClick;

    if (tabBar && (mousRel || dblClick)) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(e);
        HelpViewer *viewer = helpViewerFromTabPosition(tabWidget, mouseEvent->pos());
        if (tabWidget->count() <= 1)
            return QWidget::eventFilter(object, e);

        if (viewer && (mouseEvent->button() == Qt::MidButton || dblClick)) {
            tabWidget->removeTab(tabWidget->indexOf(viewer));
            QTimer::singleShot(0, viewer, SLOT(deleteLater()));
            currentPageChanged(tabWidget->currentIndex());
            return true;
        }
    }
    return QWidget::eventFilter(object, e);
}

bool CentralWidget::find(const QString &txt, QTextDocument::FindFlags findFlags,
    bool incremental)
{
    HelpViewer* viewer = currentHelpViewer();

#if !defined(QT_NO_WEBKIT)
    Q_UNUSED(incremental);
    if (viewer) {
        QWebPage::FindFlags options = QWebPage::FindWrapsAroundDocument;
        if (findFlags & QTextDocument::FindBackward)
            options |= QWebPage::FindBackward;
        if (findFlags & QTextDocument::FindCaseSensitively)
            options |= QWebPage::FindCaseSensitively;

        return viewer->findText(txt, options);
    }
    return false;
#else
    QTextCursor cursor;
    QTextDocument *doc = 0;
    QTextBrowser *browser = 0;

    if (viewer) {
        doc = viewer->document();
        cursor = viewer->textCursor();
        browser = qobject_cast<QTextBrowser*>(viewer);
    }
    /*
    if (tabWidget->currentWidget() == m_searchWidget) {
        QTextBrowser* browser = qFindChild<QTextBrowser*>(m_searchWidget);
        if (browser) {
            doc = browser->document();
            cursor = browser->textCursor();
        }
    }
    */
    if (!browser || !doc || cursor.isNull())
        return false;
    if (incremental)
        cursor.setPosition(cursor.selectionStart());

    QTextCursor found = doc->find(txt, cursor, findFlags);
    if (found.isNull()) {
        if ((findFlags&QTextDocument::FindBackward) == 0)
            cursor.movePosition(QTextCursor::Start);
        else
            cursor.movePosition(QTextCursor::End);
        found = doc->find(txt, cursor, findFlags);
        if (found.isNull()) {
            return false;
        }
    }
    if (!found.isNull()) {
        viewer->setTextCursor(found);
    }
    return true;
#endif
}

void CentralWidget::showTopicChooser(const QMap<QString, QUrl> &links,
    const QString &keyword)
{
    TopicChooser tc(this, keyword, links);
    if (tc.exec() == QDialog::Accepted)
        setSource(tc.link());
}

void CentralWidget::copy()
{
    HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        viewer->copy();
}

QString CentralWidget::quoteTabTitle(const QString &title) const
{
    QString s = title;
    return s.replace(QLatin1Char('&'), QLatin1String("&&"));
}
