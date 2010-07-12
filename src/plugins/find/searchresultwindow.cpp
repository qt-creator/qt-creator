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

#include "searchresultwindow.h"
#include "searchresulttreemodel.h"
#include "searchresulttreeitems.h"
#include "searchresulttreeview.h"
#include "ifindsupport.h"

#include <aggregation/aggregate.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <utils/qtcassert.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtGui/QListWidget>
#include <QtGui/QToolButton>
#include <QtGui/QLineEdit>
#include <QtGui/QStackedWidget>
#include <QtGui/QLabel>
#include <QtGui/QFont>
#include <QtGui/QAction>

static const char SETTINGSKEYSECTIONNAME[] = "SearchResults";
static const char SETTINGSKEYEXPANDRESULTS[] = "ExpandResults";


namespace Find {

namespace Internal {

    class WideEnoughLineEdit : public QLineEdit {
        Q_OBJECT
    public:
        WideEnoughLineEdit(QWidget *parent):QLineEdit(parent){
            connect(this, SIGNAL(textChanged(QString)),
                    this, SLOT(updateGeometry()));
        }
        ~WideEnoughLineEdit(){}
        QSize sizeHint() const {
            QSize sh = QLineEdit::minimumSizeHint();
            sh.rwidth() += qMax(25 * fontMetrics().width(QLatin1Char('x')),
                                fontMetrics().width(text()));
            return sh;
        }
    public slots:
        void updateGeometry() { QLineEdit::updateGeometry(); }
    };

    class SearchResultFindSupport : public IFindSupport
    {
        Q_OBJECT
    public:
        SearchResultFindSupport(SearchResultTreeView *view)
            : m_view(view)
        {
        }

        bool supportsReplace() const { return false; }

        IFindSupport::FindFlags supportedFindFlags() const
        {
            return IFindSupport::FindBackward | IFindSupport::FindCaseSensitively
                    | IFindSupport::FindRegularExpression | IFindSupport::FindWholeWords;
        }

        void resetIncrementalSearch()
        {
            m_incrementalFindStart = QModelIndex();
        }

        void clearResults() { }

        QString currentFindString() const
        {
            return QString();
        }

        QString completedFindString() const
        {
            return QString();
        }

        void highlightAll(const QString &txt, IFindSupport::FindFlags findFlags)
        {
            Q_UNUSED(txt)
            Q_UNUSED(findFlags)
            return;
        }

        IFindSupport::Result findIncremental(const QString &txt, IFindSupport::FindFlags findFlags)
        {
            if (!m_incrementalFindStart.isValid())
                m_incrementalFindStart = m_view->currentIndex();
            m_view->setCurrentIndex(m_incrementalFindStart);
            return find(txt, findFlags);
        }

        IFindSupport::Result findStep(const QString &txt, IFindSupport::FindFlags findFlags)
        {
            IFindSupport::Result result = find(txt, findFlags);
            if (result == IFindSupport::Found)
                m_incrementalFindStart = m_view->currentIndex();
            return result;
        }

        IFindSupport::Result find(const QString &txt, IFindSupport::FindFlags findFlags)
        {
            if (txt.isEmpty())
                return IFindSupport::NotFound;
            QModelIndex index;
            if (findFlags & IFindSupport::FindRegularExpression) {
                bool sensitive = (findFlags & IFindSupport::FindCaseSensitively);
                index = m_view->model()->find(QRegExp(txt, (sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive)),
                                      m_view->currentIndex(),
                                      IFindSupport::textDocumentFlagsForFindFlags(findFlags));
            } else {
                index = m_view->model()->find(txt, m_view->currentIndex(),
                                      IFindSupport::textDocumentFlagsForFindFlags(findFlags));
            }
            if (index.isValid()) {
                m_view->setCurrentIndex(index);
                m_view->scrollTo(index);
                if (index.parent().isValid())
                    m_view->expand(index.parent());
                return IFindSupport::Found;
            }
            return IFindSupport::NotFound;
        }

        bool replaceStep(const QString &before, const QString &after,
            IFindSupport::FindFlags findFlags)
        {
            Q_UNUSED(before)
            Q_UNUSED(after)
            Q_UNUSED(findFlags)
            return false;
        }

        int replaceAll(const QString &before, const QString &after,
            IFindSupport::FindFlags findFlags)
        {
            Q_UNUSED(before)
            Q_UNUSED(after)
            Q_UNUSED(findFlags)
            return 0;
        }

    private:
        SearchResultTreeView *m_view;
        QModelIndex m_incrementalFindStart;
    };

    struct SearchResultWindowPrivate {
        SearchResultWindowPrivate();

        Internal::SearchResultTreeView *m_searchResultTreeView;
        QListWidget *m_noMatchesFoundDisplay;
        QToolButton *m_expandCollapseButton;
        QAction *m_expandCollapseAction;
        QLabel *m_replaceLabel;
        QLineEdit *m_replaceTextEdit;
        QToolButton *m_replaceButton;
        static const bool m_initiallyExpand = false;
        QStackedWidget *m_widget;
        SearchResult *m_currentSearch;
        QList<SearchResultItem> m_items;
        bool m_isShowingReplaceUI;
        bool m_focusReplaceEdit;
    };

    SearchResultWindowPrivate::SearchResultWindowPrivate()
        : m_currentSearch(0),
        m_isShowingReplaceUI(false),
        m_focusReplaceEdit(false)
    {
    }
}

using namespace Find::Internal;

SearchResultWindow *SearchResultWindow::m_instance = 0;

SearchResultWindow::SearchResultWindow() : d(new SearchResultWindowPrivate)
{
    m_instance = this;
    d->m_widget = new QStackedWidget;
    d->m_widget->setWindowTitle(displayName());

    d->m_searchResultTreeView = new Internal::SearchResultTreeView(d->m_widget);
    d->m_searchResultTreeView->setFrameStyle(QFrame::NoFrame);
    d->m_searchResultTreeView->setAttribute(Qt::WA_MacShowFocusRect, false);
    d->m_widget->addWidget(d->m_searchResultTreeView);
    Aggregation::Aggregate * agg = new Aggregation::Aggregate;
    agg->add(d->m_searchResultTreeView);
    agg->add(new SearchResultFindSupport(d->m_searchResultTreeView));

    d->m_noMatchesFoundDisplay = new QListWidget(d->m_widget);
    d->m_noMatchesFoundDisplay->addItem(tr("No matches found!"));
    d->m_noMatchesFoundDisplay->setFrameStyle(QFrame::NoFrame);
    d->m_widget->addWidget(d->m_noMatchesFoundDisplay);

    d->m_expandCollapseButton = new QToolButton(d->m_widget);
    d->m_expandCollapseButton->setAutoRaise(true);

    d->m_expandCollapseAction = new QAction(tr("Expand All"), this);
    d->m_expandCollapseAction->setCheckable(true);
    d->m_expandCollapseAction->setIcon(QIcon(QLatin1String(":/find/images/expand.png")));
    Core::Command *cmd = Core::ICore::instance()->actionManager()->registerAction(
            d->m_expandCollapseAction, QLatin1String("Find.ExpandAll"),
            Core::Context(Core::Constants::C_GLOBAL));
    d->m_expandCollapseButton->setDefaultAction(cmd->action());

    d->m_replaceLabel = new QLabel(tr("Replace with:"), d->m_widget);
    d->m_replaceLabel->setContentsMargins(12, 0, 5, 0);
    d->m_replaceTextEdit = new WideEnoughLineEdit(d->m_widget);
    d->m_replaceButton = new QToolButton(d->m_widget);
    d->m_replaceButton->setToolTip(tr("Replace all occurrences"));
    d->m_replaceButton->setText(tr("Replace"));
    d->m_replaceButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    d->m_replaceButton->setAutoRaise(true);
    d->m_replaceTextEdit->setTabOrder(d->m_replaceTextEdit, d->m_searchResultTreeView);

    connect(d->m_searchResultTreeView, SIGNAL(jumpToSearchResult(int,bool)),
            this, SLOT(handleJumpToSearchResult(int,bool)));
    connect(d->m_expandCollapseAction, SIGNAL(toggled(bool)), this, SLOT(handleExpandCollapseToolButton(bool)));
    connect(d->m_replaceTextEdit, SIGNAL(returnPressed()), this, SLOT(handleReplaceButton()));
    connect(d->m_replaceButton, SIGNAL(clicked()), this, SLOT(handleReplaceButton()));

    readSettings();
    setShowReplaceUI(false);
}

SearchResultWindow::~SearchResultWindow()
{
    writeSettings();
    delete d->m_currentSearch;
    d->m_currentSearch = 0;
    delete d->m_widget;
    d->m_widget = 0;
    d->m_items.clear();
    delete d;
}

SearchResultWindow *SearchResultWindow::instance()
{
    return m_instance;
}

void SearchResultWindow::setTextToReplace(const QString &textToReplace)
{
    d->m_replaceTextEdit->setText(textToReplace);
}

QString SearchResultWindow::textToReplace() const
{
    return d->m_replaceTextEdit->text();
}

void SearchResultWindow::setShowReplaceUI(bool show)
{
    d->m_searchResultTreeView->model()->setShowReplaceUI(show);
    d->m_replaceLabel->setVisible(show);
    d->m_replaceTextEdit->setVisible(show);
    d->m_replaceButton->setVisible(show);
    d->m_isShowingReplaceUI = show;
}

void SearchResultWindow::handleReplaceButton()
{
    QTC_ASSERT(d->m_currentSearch, return);
    // check if button is actually enabled, because this is also triggered
    // by pressing return in replace line edit
    if (d->m_replaceButton->isEnabled())
        d->m_currentSearch->replaceButtonClicked(d->m_replaceTextEdit->text(), checkedItems());
}

QList<SearchResultItem> SearchResultWindow::checkedItems() const
{
    QList<SearchResultItem> result;
    Internal::SearchResultTreeModel *model = d->m_searchResultTreeView->model();
    const int fileCount = model->rowCount(QModelIndex());
    for (int i = 0; i < fileCount; ++i) {
        QModelIndex fileIndex = model->index(i, 0, QModelIndex());
        Internal::SearchResultFile *fileItem = static_cast<Internal::SearchResultFile *>(fileIndex.internalPointer());
        Q_ASSERT(fileItem != 0);
        for (int rowIndex = 0; rowIndex < fileItem->childrenCount(); ++rowIndex) {
            QModelIndex textIndex = model->index(rowIndex, 0, fileIndex);
            Internal::SearchResultTextRow *rowItem = static_cast<Internal::SearchResultTextRow *>(textIndex.internalPointer());
            if (rowItem->checkState())
                result << d->m_items.at(rowItem->index());
        }
    }
    return result;
}

void SearchResultWindow::visibilityChanged(bool /*visible*/)
{
}

QWidget *SearchResultWindow::outputWidget(QWidget *)
{
    return d->m_widget;
}

QList<QWidget*> SearchResultWindow::toolBarWidgets() const
{
    return QList<QWidget*>() << d->m_expandCollapseButton << d->m_replaceLabel << d->m_replaceTextEdit << d->m_replaceButton;
}

SearchResult *SearchResultWindow::startNewSearch(SearchMode searchOrSearchAndReplace)
{
    clearContents();
    setShowReplaceUI(searchOrSearchAndReplace != SearchOnly);
    delete d->m_currentSearch;
    d->m_currentSearch = new SearchResult;
    return d->m_currentSearch;
}

void SearchResultWindow::finishSearch()
{
    if (d->m_items.count()) {
        d->m_replaceButton->setEnabled(true);
    } else {
        showNoMatchesFound();
    }
}

void SearchResultWindow::clearContents()
{
    d->m_replaceTextEdit->setEnabled(false);
    d->m_replaceButton->setEnabled(false);
    d->m_replaceTextEdit->clear();
    d->m_searchResultTreeView->clear();
    d->m_items.clear();
    d->m_widget->setCurrentWidget(d->m_searchResultTreeView);
    navigateStateChanged();
}

void SearchResultWindow::showNoMatchesFound()
{
    d->m_replaceTextEdit->setEnabled(false);
    d->m_replaceButton->setEnabled(false);
    d->m_widget->setCurrentWidget(d->m_noMatchesFoundDisplay);
}

bool SearchResultWindow::isEmpty() const
{
    return (d->m_searchResultTreeView->model()->rowCount() < 1);
}

int SearchResultWindow::numberOfResults() const
{
    return d->m_items.count();
}

bool SearchResultWindow::hasFocus()
{
    return d->m_searchResultTreeView->hasFocus() || (d->m_isShowingReplaceUI && d->m_replaceTextEdit->hasFocus());
}

bool SearchResultWindow::canFocus()
{
    return !d->m_items.isEmpty();
}

void SearchResultWindow::setFocus()
{
    if (!d->m_items.isEmpty()) {
        if (!d->m_isShowingReplaceUI) {
            d->m_searchResultTreeView->setFocus();
        } else {
            if (!d->m_widget->focusWidget()
                    || d->m_widget->focusWidget() == d->m_replaceTextEdit
                    || d->m_focusReplaceEdit) {
                d->m_replaceTextEdit->setFocus();
                d->m_replaceTextEdit->selectAll();
            } else {
                d->m_searchResultTreeView->setFocus();
            }
        }
    }
}

void SearchResultWindow::setTextEditorFont(const QFont &font)
{
    d->m_searchResultTreeView->setTextEditorFont(font);
}

void SearchResultWindow::handleJumpToSearchResult(int index, bool /* checked */)
{
    QTC_ASSERT(d->m_currentSearch, return);
    d->m_currentSearch->activated(d->m_items.at(index));
}

void SearchResultWindow::addResult(const QString &fileName, int lineNumber, const QString &rowText,
    int searchTermStart, int searchTermLength, const QVariant &userData)
{
    SearchResultItem item;
    item.fileName = fileName;
    item.lineNumber = lineNumber;
    item.lineText = rowText;
    item.searchTermStart = searchTermStart;
    item.searchTermLength = searchTermLength;
    item.userData = userData;
    addResults(QList<SearchResultItem>() << item);
}

void SearchResultWindow::addResults(QList<SearchResultItem> &items)
{
    int index = d->m_items.size();
    bool firstItems = (index == 0);
    for (int i = 0; i < items.size(); ++i) {
        items[i].index = index;
        ++index;
    }

    d->m_items << items;
    d->m_searchResultTreeView->appendResultLines(items);
    if (firstItems) {
        d->m_replaceTextEdit->setEnabled(true);
        // We didn't have an item before, set the focus to the search widget
        d->m_focusReplaceEdit = true;
        setFocus();
        d->m_focusReplaceEdit = false;
        d->m_searchResultTreeView->selectionModel()->select(d->m_searchResultTreeView->model()->index(0, 0, QModelIndex()), QItemSelectionModel::Select);
        emit navigateStateChanged();
    }
}

void SearchResultWindow::handleExpandCollapseToolButton(bool checked)
{
    d->m_searchResultTreeView->setAutoExpandResults(checked);
    if (checked)
        d->m_searchResultTreeView->expandAll();
    else
        d->m_searchResultTreeView->collapseAll();
}

void SearchResultWindow::readSettings()
{
    QSettings *s = Core::ICore::instance()->settings();
    if (s) {
        s->beginGroup(QLatin1String(SETTINGSKEYSECTIONNAME));
        d->m_expandCollapseAction->setChecked(s->value(QLatin1String(SETTINGSKEYEXPANDRESULTS), d->m_initiallyExpand).toBool());
        s->endGroup();
    }
}

void SearchResultWindow::writeSettings()
{
    QSettings *s = Core::ICore::instance()->settings();
    if (s) {
        s->beginGroup(QLatin1String(SETTINGSKEYSECTIONNAME));
        s->setValue(QLatin1String(SETTINGSKEYEXPANDRESULTS), d->m_expandCollapseAction->isChecked());
        s->endGroup();
    }
}

int SearchResultWindow::priorityInStatusBar() const
{
    return 80;
}

bool SearchResultWindow::canNext()
{
    return d->m_items.count() > 0;
}

bool SearchResultWindow::canPrevious()
{
    return d->m_items.count() > 0;
}

void SearchResultWindow::goToNext()
{
    if (d->m_items.count() == 0)
        return;
    QModelIndex idx = d->m_searchResultTreeView->model()->next(d->m_searchResultTreeView->currentIndex());
    if (idx.isValid()) {
        d->m_searchResultTreeView->setCurrentIndex(idx);
        d->m_searchResultTreeView->emitJumpToSearchResult(idx);
    }
}
void SearchResultWindow::goToPrev()
{
    if (!d->m_searchResultTreeView->model()->rowCount())
        return;
    QModelIndex idx = d->m_searchResultTreeView->model()->prev(d->m_searchResultTreeView->currentIndex());
    if (idx.isValid()) {
        d->m_searchResultTreeView->setCurrentIndex(idx);
        d->m_searchResultTreeView->emitJumpToSearchResult(idx);
    }
}

bool SearchResultWindow::canNavigate()
{
    return true;
}

} // namespace Find


#include "searchresultwindow.moc"
