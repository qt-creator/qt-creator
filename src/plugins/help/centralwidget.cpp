/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "centralwidget.h"

#include "helpmanager.h"
#include "helpviewer.h"
#include "topicchooser.h"

#include <QtCore/QEvent>
#include <QtCore/QTimer>

#include <QtGui/QKeyEvent>
#include <QtGui/QLayout>
#include <QtGui/QPageSetupDialog>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>
#include <QtGui/QPrintPreviewDialog>
#include <QtGui/QStackedWidget>

#include <QtHelp/QHelpEngine>
#include <QtHelp/QHelpEngineCore>
#include <QtHelp/QHelpSearchEngine>

using namespace Help::Internal;

CentralWidget *gStaticCentralWidget = 0;

// -- CentralWidget

CentralWidget::CentralWidget(QWidget *parent)
    : QWidget(parent)
    , printer(0)
    , m_stackedWidget(0)
{
    Q_ASSERT(!gStaticCentralWidget);
    gStaticCentralWidget = this;

    QVBoxLayout *vboxLayout = new QVBoxLayout(this);
    vboxLayout->setMargin(0);
    m_stackedWidget = new QStackedWidget(this);
    vboxLayout->addWidget(m_stackedWidget);
}

CentralWidget::~CentralWidget()
{
#ifndef QT_NO_PRINTER
    delete printer;
#endif

    QString zoomFactors;
    QString currentPages;
    for (int i = 0; i < m_stackedWidget->count(); ++i) {
        const HelpViewer * const viewer = viewerAt(i);
        const QUrl &source = viewer->source();
        if (source.isValid()) {
            currentPages += source.toString() + QLatin1Char('|');
            zoomFactors += QString::number(viewer->scale()) + QLatin1Char('|');
        }
    }

    QHelpEngineCore *engine = &HelpManager::helpEngineCore();
    engine->setCustomValue(QLatin1String("LastShownPages"), currentPages);
    engine->setCustomValue(QLatin1String("LastShownPagesZoom"), zoomFactors);
    engine->setCustomValue(QLatin1String("LastTabPage"), currentIndex());
}

CentralWidget *CentralWidget::instance()
{
    Q_ASSERT(gStaticCentralWidget);
    return gStaticCentralWidget;
}

bool CentralWidget::hasSelection() const
{
    if (HelpViewer* viewer = currentHelpViewer())
        return !viewer->selectedText().isEmpty();
    return false;
}

bool CentralWidget::isForwardAvailable() const
{
    const HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        return viewer->isForwardAvailable();

    return false;
}

bool CentralWidget::isBackwardAvailable() const
{
    const HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        return viewer->isBackwardAvailable();

    return false;
}

HelpViewer* CentralWidget::viewerAt(int index) const
{
    return qobject_cast<HelpViewer*> (m_stackedWidget->widget(index));
}

HelpViewer* CentralWidget::currentHelpViewer() const
{
    return qobject_cast<HelpViewer*> (m_stackedWidget->currentWidget());
}

void CentralWidget::addPage(HelpViewer *page, bool fromSearch)
{
    page->installEventFilter(this);
    page->setFocus(Qt::OtherFocusReason);
    connectSignals(page);
    m_stackedWidget->addWidget(page);
    if (fromSearch) {
        connect(currentHelpViewer(), SIGNAL(loadFinished(bool)), this,
            SLOT(highlightSearchTerms()));
     }
}

void CentralWidget::removePage(int index)
{
    const bool currentChanged = (index == currentIndex());
    m_stackedWidget->removeWidget(m_stackedWidget->widget(index));
    if (currentChanged)
        emit currentViewerChanged();
}

int CentralWidget::currentIndex() const
{
    return  m_stackedWidget->currentIndex();
}

void CentralWidget::setCurrentPage(HelpViewer *page)
{
    m_stackedWidget->setCurrentWidget(page);
    emit currentViewerChanged();
}

bool CentralWidget::find(const QString &txt, Find::IFindSupport::FindFlags flags,
    bool incremental)
{
    return currentHelpViewer()->findText(txt, flags, incremental, false);
}

// -- public slots

void CentralWidget::copy()
{
    if (HelpViewer* viewer = currentHelpViewer())
        viewer->copy();
}

void CentralWidget::home()
{
    if (HelpViewer* viewer = currentHelpViewer())
        viewer->home();
}

void CentralWidget::zoomIn()
{
    HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        viewer->scaleUp();
}

void CentralWidget::zoomOut()
{
    HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        viewer->scaleDown();
}

void CentralWidget::resetZoom()
{
    HelpViewer* viewer = currentHelpViewer();
    if (viewer)
        viewer->resetScale();
}

void CentralWidget::forward()
{
    if (HelpViewer* viewer = currentHelpViewer())
        viewer->forward();
}

void CentralWidget::nextPage()
{
    m_stackedWidget->setCurrentIndex((m_stackedWidget->currentIndex() + 1)
        % m_stackedWidget->count());
}

void CentralWidget::backward()
{
    if (HelpViewer* viewer = currentHelpViewer())
        viewer->backward();
}

void CentralWidget::previousPage()
{
    m_stackedWidget->setCurrentIndex((m_stackedWidget->currentIndex() - 1)
        % m_stackedWidget->count());
}

void CentralWidget::print()
{
#ifndef QT_NO_PRINTER
    if (HelpViewer* viewer = currentHelpViewer()) {
        initPrinter();

        QPrintDialog *dlg = new QPrintDialog(printer, this);
        dlg->setWindowTitle(tr("Print Document"));
        if (!viewer->selectedText().isEmpty())
            dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
        dlg->addEnabledOption(QAbstractPrintDialog::PrintPageRange);
        dlg->addEnabledOption(QAbstractPrintDialog::PrintCollateCopies);
    
        if (dlg->exec() == QDialog::Accepted)
            viewer->print(printer);
        delete dlg;
    }
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

void CentralWidget::printPreview()
{
#ifndef QT_NO_PRINTER
    initPrinter();
    QPrintPreviewDialog preview(printer, this);
    connect(&preview, SIGNAL(paintRequested(QPrinter*)),
        SLOT(printPreview(QPrinter*)));
    preview.exec();
#endif
}

void CentralWidget::setSource(const QUrl &url)
{
    if (HelpViewer* viewer = currentHelpViewer()) {
        viewer->setSource(url);
        viewer->setFocus(Qt::OtherFocusReason);
    }
}

void CentralWidget::setSourceFromSearch(const QUrl &url)
{
    if (HelpViewer* viewer = currentHelpViewer()) {
        connect(viewer, SIGNAL(loadFinished(bool)), this,
            SLOT(highlightSearchTerms()));
        viewer->setSource(url);
        viewer->setFocus(Qt::OtherFocusReason);
    }
}

void CentralWidget::showTopicChooser(const QMap<QString, QUrl> &links,
    const QString &keyword)
{
    TopicChooser tc(this, keyword, links);
    if (tc.exec() == QDialog::Accepted)
        setSource(tc.link());
}

// -- protected

void CentralWidget::focusInEvent(QFocusEvent * /* event */)
{
    // If we have a current help viewer then this is the 'focus proxy',
    // otherwise it's the central widget. This is needed, so an embedding
    // program can just set the focus to the central widget and it does
    // The Right Thing(TM)
    QObject *receiver = m_stackedWidget;
    if (HelpViewer *viewer = currentHelpViewer())
        receiver = viewer;
    QTimer::singleShot(1, receiver, SLOT(setFocus()));
}

// -- private slots

void CentralWidget::highlightSearchTerms()
{
    if (HelpViewer *viewer = currentHelpViewer()) {
        QHelpSearchEngine *searchEngine = 
            HelpManager::instance().helpEngine().searchEngine();
        QList<QHelpSearchQuery> queryList = searchEngine->query();

        QStringList terms;
        foreach (const QHelpSearchQuery &query, queryList) {
            switch (query.fieldName) {
                default: break;
                case QHelpSearchQuery::ALL: {
                case QHelpSearchQuery::PHRASE:
                case QHelpSearchQuery::DEFAULT:
                case QHelpSearchQuery::ATLEAST:
                    foreach (QString term, query.wordList)
                        terms.append(term.remove(QLatin1String("\"")));
                }
            }
        }

        foreach (const QString& term, terms)
            viewer->findText(term, 0, false, true);
        disconnect(viewer, SIGNAL(loadFinished(bool)), this,
            SLOT(highlightSearchTerms()));
    }
}

void CentralWidget::printPreview(QPrinter *p)
{
#ifndef QT_NO_PRINTER
    HelpViewer *viewer = currentHelpViewer();
    if (viewer)
        viewer->print(p);
#else
    Q_UNUSED(p)
#endif
}

void CentralWidget::handleSourceChanged(const QUrl &url)
{
    if (sender() == currentHelpViewer())
        emit sourceChanged(url);
}

// -- private

void CentralWidget::initPrinter()
{
#ifndef QT_NO_PRINTER
    if (!printer)
        printer = new QPrinter(QPrinter::HighResolution);
#endif
}

void CentralWidget::connectSignals(HelpViewer *page)
{
    connect(page, SIGNAL(sourceChanged(QUrl)), this, SLOT(handleSourceChanged(QUrl)));
    connect(page, SIGNAL(forwardAvailable(bool)), this, SIGNAL(forwardAvailable(bool)));
    connect(page, SIGNAL(backwardAvailable(bool)), this, SIGNAL(backwardAvailable(bool)));
}

bool CentralWidget::eventFilter(QObject *object, QEvent *e)
{
    if (e->type() != QEvent::KeyPress)
        return QWidget::eventFilter(object, e);

    HelpViewer *viewer = currentHelpViewer();
    QKeyEvent *keyEvent = static_cast<QKeyEvent*> (e);
    if (viewer == object && keyEvent->key() == Qt::Key_Backspace) {
        if (viewer->isBackwardAvailable()) {
#if !defined(QT_NO_WEBKIT)
            // this helps in case there is an html <input> field
            if (!viewer->hasFocus())
#endif
                viewer->backward();
        }
    }
    return QWidget::eventFilter(object, e);
}
