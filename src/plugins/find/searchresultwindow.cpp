/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
#include <coreplugin/uniqueidmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtCore/QDir>
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

        Find::FindFlags supportedFindFlags() const
        {
            return Find::FindBackward | Find::FindCaseSensitively
                    | Find::FindRegularExpression | Find::FindWholeWords;
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

        void highlightAll(const QString &txt, Find::FindFlags findFlags)
        {
            Q_UNUSED(txt)
            Q_UNUSED(findFlags)
            return;
        }

        IFindSupport::Result findIncremental(const QString &txt, Find::FindFlags findFlags)
        {
            if (!m_incrementalFindStart.isValid())
                m_incrementalFindStart = m_view->currentIndex();
            m_view->setCurrentIndex(m_incrementalFindStart);
            return find(txt, findFlags);
        }

        IFindSupport::Result findStep(const QString &txt, Find::FindFlags findFlags)
        {
            IFindSupport::Result result = find(txt, findFlags);
            if (result == IFindSupport::Found)
                m_incrementalFindStart = m_view->currentIndex();
            return result;
        }

        IFindSupport::Result find(const QString &txt, Find::FindFlags findFlags)
        {
            if (txt.isEmpty())
                return IFindSupport::NotFound;
            QModelIndex index;
            if (findFlags & Find::FindRegularExpression) {
                bool sensitive = (findFlags & Find::FindCaseSensitively);
                index = m_view->model()->find(QRegExp(txt, (sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive)),
                                      m_view->currentIndex(),
                                      Find::textDocumentFlagsForFindFlags(findFlags));
            } else {
                index = m_view->model()->find(txt, m_view->currentIndex(),
                                      Find::textDocumentFlagsForFindFlags(findFlags));
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

        void replace(const QString &before, const QString &after,
            Find::FindFlags findFlags)
        {
            Q_UNUSED(before)
            Q_UNUSED(after)
            Q_UNUSED(findFlags)
        }

        bool replaceStep(const QString &before, const QString &after,
            Find::FindFlags findFlags)
        {
            Q_UNUSED(before)
            Q_UNUSED(after)
            Q_UNUSED(findFlags)
            return false;
        }

        int replaceAll(const QString &before, const QString &after,
            Find::FindFlags findFlags)
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
        int m_itemCount;
        bool m_isShowingReplaceUI;
        bool m_focusReplaceEdit;
    };

    SearchResultWindowPrivate::SearchResultWindowPrivate()
        : m_currentSearch(0),
        m_itemCount(0),
        m_isShowingReplaceUI(false),
        m_focusReplaceEdit(false)
    {
    }
}

using namespace Find::Internal;

/*!
    \enum Find::SearchResultWindow::SearchMode
    Specifies if a search should show the replace UI or not.

    \value SearchOnly
           The search doesn't support replace.
    \value SearchAndReplace
           The search supports replace, so show the UI for it.
*/

/*!
    \class Find::SearchResult
    \brief Reports user interaction like activation of a search result item.

    Whenever a new search is initiated via startNewSearch, an instance of this
    class is returned to provide the initiator with the hooks for handling user
    interaction.
*/

/*!
    \fn void SearchResult::activated(const Find::SearchResultItem &item)
    \brief Sent if the user activated (e.g. double-clicked) a search result
    \a item.
*/

/*!
    \fn void SearchResult::replaceButtonClicked(const QString &replaceText, const QList<Find::SearchResultItem> &checkedItems)
    \brief Sent when the user initiated a replace, e.g. by pressing the replace
    all button.

    The signal reports the text to use for replacement in \a replaceText,
    and the list of search result items that were selected by the user
    in \a checkedItems.
    The handler of this signal should apply the replace only on the selected
    items.
*/

/*!
    \class Find::SearchResultWindow
    \brief The SearchResultWindow class is the implementation of a commonly
    shared \gui{Search Results} output pane. Use it to show search results
    to a user.

    Whenever you want to show the user a list of search results, or want
    to present UI for a global search and replace, use the single instance
    of this class.

    Except for being an implementation of a output pane, the
    SearchResultWindow has a few methods and one enum that allows other
    plugins to show their search results and hook into the user actions for
    selecting an entry and performing a global replace.

    Whenever you start a search, call startNewSearch(SearchMode) to initialize
    the search result window. The parameter determines if the GUI for
    replacing should be shown.
    The method returns a SearchResult object that is your
    hook into the signals from user interaction for this search.
    When you produce search results, call addResults or addResult to add them
    to the search result window.
    After the search has finished call finishSearch to inform the search
    result window about it.

    After that you get activated signals via your SearchResult instance when
    the user selects a search result item, and, if you started the search
    with the SearchAndReplace option, the replaceButtonClicked signal
    when the user requests a replace.
*/

/*!
    \fn QString SearchResultWindow::displayName() const
    \internal
*/

SearchResultWindow *SearchResultWindow::m_instance = 0;

/*!
    \fn SearchResultWindow::SearchResultWindow()
    \internal
*/
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
            d->m_expandCollapseAction, "Find.ExpandAll",
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

    connect(d->m_searchResultTreeView, SIGNAL(jumpToSearchResult(SearchResultItem)),
            this, SLOT(handleJumpToSearchResult(SearchResultItem)));
    connect(d->m_expandCollapseAction, SIGNAL(toggled(bool)), this, SLOT(handleExpandCollapseToolButton(bool)));
    connect(d->m_replaceTextEdit, SIGNAL(returnPressed()), this, SLOT(handleReplaceButton()));
    connect(d->m_replaceButton, SIGNAL(clicked()), this, SLOT(handleReplaceButton()));

    readSettings();
    setShowReplaceUI(false);
}

/*!
    \fn SearchResultWindow::~SearchResultWindow()
    \internal
*/
SearchResultWindow::~SearchResultWindow()
{
    writeSettings();
    delete d->m_currentSearch;
    d->m_currentSearch = 0;
    delete d->m_widget;
    d->m_widget = 0;
    d->m_itemCount = 0;
    delete d;
}

/*!
    \fn SearchResultWindow *SearchResultWindow::instance()
    \brief Returns the single shared instance of the Search Results window.
*/
SearchResultWindow *SearchResultWindow::instance()
{
    return m_instance;
}

/*!
    \fn void SearchResultWindow::setTextToReplace(const QString &textToReplace)
    \brief Sets the value in the UI element that allows the user to type
    the text that should replace text in search results to \a textToReplace.
*/
void SearchResultWindow::setTextToReplace(const QString &textToReplace)
{
    d->m_replaceTextEdit->setText(textToReplace);
}

/*!
    \fn QString SearchResultWindow::textToReplace() const
    \brief Returns the text that should replace the text in search results.
*/
QString SearchResultWindow::textToReplace() const
{
    return d->m_replaceTextEdit->text();
}

/*!
    \fn void SearchResultWindow::setShowReplaceUI(bool show)
    \internal
*/
void SearchResultWindow::setShowReplaceUI(bool show)
{
    d->m_searchResultTreeView->model()->setShowReplaceUI(show);
    d->m_replaceLabel->setVisible(show);
    d->m_replaceTextEdit->setVisible(show);
    d->m_replaceButton->setVisible(show);
    d->m_isShowingReplaceUI = show;
}

/*!
    \fn void SearchResultWindow::handleReplaceButton()
    \internal
*/
void SearchResultWindow::handleReplaceButton()
{
    QTC_ASSERT(d->m_currentSearch, return);
    // check if button is actually enabled, because this is also triggered
    // by pressing return in replace line edit
    if (d->m_replaceButton->isEnabled())
        d->m_currentSearch->replaceButtonClicked(d->m_replaceTextEdit->text(), checkedItems());
}

/*!
    \fn QList<SearchResultItem> SearchResultWindow::checkedItems() const
    \internal
*/
QList<SearchResultItem> SearchResultWindow::checkedItems() const
{
    QList<SearchResultItem> result;
    Internal::SearchResultTreeModel *model = d->m_searchResultTreeView->model();
    const int fileCount = model->rowCount(QModelIndex());
    for (int i = 0; i < fileCount; ++i) {
        QModelIndex fileIndex = model->index(i, 0, QModelIndex());
        Internal::SearchResultTreeItem *fileItem = static_cast<Internal::SearchResultTreeItem *>(fileIndex.internalPointer());
        Q_ASSERT(fileItem != 0);
        for (int rowIndex = 0; rowIndex < fileItem->childrenCount(); ++rowIndex) {
            QModelIndex textIndex = model->index(rowIndex, 0, fileIndex);
            Internal::SearchResultTreeItem *rowItem = static_cast<Internal::SearchResultTreeItem *>(textIndex.internalPointer());
            if (rowItem->checkState())
                result << rowItem->item;
        }
    }
    return result;
}

/*!
    \fn void SearchResultWindow::visibilityChanged(bool)
    \internal
*/
void SearchResultWindow::visibilityChanged(bool /*visible*/)
{
}

/*!
    \fn QWidget *SearchResultWindow::outputWidget(QWidget *)
    \internal
*/
QWidget *SearchResultWindow::outputWidget(QWidget *)
{
    return d->m_widget;
}

/*!
    \fn QList<QWidget*> SearchResultWindow::toolBarWidgets() const
    \internal
*/
QList<QWidget*> SearchResultWindow::toolBarWidgets() const
{
    return QList<QWidget*>() << d->m_expandCollapseButton << d->m_replaceLabel << d->m_replaceTextEdit << d->m_replaceButton;
}

/*!
    \fn SearchResult *SearchResultWindow::startNewSearch(SearchMode searchOrSearchAndReplace)
    \brief Tells the search results window to start a new search.

    This will clear the contents of the previous search and initialize the UI
    with regard to showing the replace UI or not (depending on the search mode
    in \a searchOrSearchAndReplace).
    Returns a SearchResult object that is used for signaling user interaction
    with the results of this search.
*/
SearchResult *SearchResultWindow::startNewSearch(SearchMode searchOrSearchAndReplace)
{
    clearContents();
    setShowReplaceUI(searchOrSearchAndReplace != SearchOnly);
    delete d->m_currentSearch;
    d->m_currentSearch = new SearchResult;
    return d->m_currentSearch;
}

/*!
    \fn void SearchResultWindow::finishSearch()
    \brief Notifies the search result window that the current search
    has finished, and the UI should reflect that.
*/
void SearchResultWindow::finishSearch()
{
    if (d->m_itemCount > 0) {
        d->m_replaceButton->setEnabled(true);
    } else {
        showNoMatchesFound();
    }
}

/*!
    \fn void SearchResultWindow::clearContents()
    \brief Clears the current contents in the search result window.
*/
void SearchResultWindow::clearContents()
{
    d->m_replaceTextEdit->setEnabled(false);
    d->m_replaceButton->setEnabled(false);
    d->m_replaceTextEdit->clear();
    d->m_searchResultTreeView->clear();
    d->m_itemCount = 0;
    d->m_widget->setCurrentWidget(d->m_searchResultTreeView);
    navigateStateChanged();
}

/*!
    \fn void SearchResultWindow::showNoMatchesFound()
    \internal
*/
void SearchResultWindow::showNoMatchesFound()
{
    d->m_replaceTextEdit->setEnabled(false);
    d->m_replaceButton->setEnabled(false);
    d->m_widget->setCurrentWidget(d->m_noMatchesFoundDisplay);
}

/*!
    \fn bool SearchResultWindow::isEmpty() const
    Returns if the search result window currently doesn't show any results.
*/
bool SearchResultWindow::isEmpty() const
{
    return (d->m_searchResultTreeView->model()->rowCount() < 1);
}

/*!
    \fn int SearchResultWindow::numberOfResults() const
    Returns the number of search results currently shown in the search
    results window.
*/
int SearchResultWindow::numberOfResults() const
{
    return d->m_itemCount;
}

/*!
    \fn bool SearchResultWindow::hasFocus()
    \internal
*/
bool SearchResultWindow::hasFocus()
{
    return d->m_searchResultTreeView->hasFocus() || (d->m_isShowingReplaceUI && d->m_replaceTextEdit->hasFocus());
}

/*!
    \fn bool SearchResultWindow::canFocus()
    \internal
*/
bool SearchResultWindow::canFocus()
{
    return d->m_itemCount > 0;
}

/*!
    \fn void SearchResultWindow::setFocus()
    \internal
*/
void SearchResultWindow::setFocus()
{
    if (d->m_itemCount > 0) {
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

/*!
    \fn void SearchResultWindow::setTextEditorFont(const QFont &font)
    \internal
*/
void SearchResultWindow::setTextEditorFont(const QFont &font)
{
    d->m_searchResultTreeView->setTextEditorFont(font);
}

/*!
    \fn void SearchResultWindow::handleJumpToSearchResult(int index, bool)
    \internal
*/
void SearchResultWindow::handleJumpToSearchResult(const SearchResultItem &item)
{
    QTC_ASSERT(d->m_currentSearch, return);
    d->m_currentSearch->activated(item);
}

/*!
    \fn void SearchResultWindow::addResult(const QString &fileName, int lineNumber, const QString &rowText, int searchTermStart, int searchTermLength, const QVariant &userData)
    \brief Adds a single result line to the search results.

    The \a fileName, \a lineNumber and \a rowText are shown in the result line.
    \a searchTermStart and \a searchTermLength specify the region that
    should be visually marked (string position and length in \a rowText).
    You can attach arbitrary \a userData to the search result, which can
    be used e.g. when reacting to the signals of the SearchResult for your search.

    \sa addResults()
*/
void SearchResultWindow::addResult(const QString &fileName, int lineNumber, const QString &rowText,
    int searchTermStart, int searchTermLength, const QVariant &userData)
{
    SearchResultItem item;
    item.path = QStringList() << QDir::toNativeSeparators(fileName);
    item.lineNumber = lineNumber;
    item.text = rowText;
    item.textMarkPos = searchTermStart;
    item.textMarkLength = searchTermLength;
    item.useTextEditorFont = true;
    item.userData = userData;
    addResults(QList<SearchResultItem>() << item, AddOrdered);
}

/*!
    \fn void SearchResultWindow::addResults(QList<SearchResultItem> &items)
    \brief Adds all of the given search result \a items to the search
    results window.

    \sa addResult()
*/
void SearchResultWindow::addResults(QList<SearchResultItem> &items, AddMode mode)
{
    bool firstItems = (d->m_itemCount == 0);
    d->m_itemCount += items.size();
    d->m_searchResultTreeView->addResults(items, mode);
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

/*!
    \fn void SearchResultWindow::handleExpandCollapseToolButton(bool checked)
    \internal
*/
void SearchResultWindow::handleExpandCollapseToolButton(bool checked)
{
    d->m_searchResultTreeView->setAutoExpandResults(checked);
    if (checked)
        d->m_searchResultTreeView->expandAll();
    else
        d->m_searchResultTreeView->collapseAll();
}

/*!
    \fn void SearchResultWindow::readSettings()
    \internal
*/
void SearchResultWindow::readSettings()
{
    QSettings *s = Core::ICore::instance()->settings();
    if (s) {
        s->beginGroup(QLatin1String(SETTINGSKEYSECTIONNAME));
        d->m_expandCollapseAction->setChecked(s->value(QLatin1String(SETTINGSKEYEXPANDRESULTS), d->m_initiallyExpand).toBool());
        s->endGroup();
    }
}

/*!
    \fn void SearchResultWindow::writeSettings()
    \internal
*/
void SearchResultWindow::writeSettings()
{
    QSettings *s = Core::ICore::instance()->settings();
    if (s) {
        s->beginGroup(QLatin1String(SETTINGSKEYSECTIONNAME));
        s->setValue(QLatin1String(SETTINGSKEYEXPANDRESULTS), d->m_expandCollapseAction->isChecked());
        s->endGroup();
    }
}

/*!
    \fn int SearchResultWindow::priorityInStatusBar() const
    \internal
*/
int SearchResultWindow::priorityInStatusBar() const
{
    return 80;
}

/*!
    \fn bool SearchResultWindow::canNext()
    \internal
*/
bool SearchResultWindow::canNext()
{
    return d->m_itemCount > 0;
}

/*!
    \fn bool SearchResultWindow::canPrevious()
    \internal
*/
bool SearchResultWindow::canPrevious()
{
    return d->m_itemCount > 0;
}

/*!
    \fn void SearchResultWindow::goToNext()
    \internal
*/
void SearchResultWindow::goToNext()
{
    if (d->m_itemCount == 0)
        return;
    QModelIndex idx = d->m_searchResultTreeView->model()->next(d->m_searchResultTreeView->currentIndex());
    if (idx.isValid()) {
        d->m_searchResultTreeView->setCurrentIndex(idx);
        d->m_searchResultTreeView->emitJumpToSearchResult(idx);
    }
}

/*!
    \fn void SearchResultWindow::goToPrev()
    \internal
*/
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

/*!
    \fn bool SearchResultWindow::canNavigate()
    \internal
*/
bool SearchResultWindow::canNavigate()
{
    return true;
}

} // namespace Find


#include "searchresultwindow.moc"
