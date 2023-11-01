// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "searchresultwindow.h"

#include "searchresultwidget.h"
#include "textfindconstants.h"
#include "../actionmanager/actionmanager.h"
#include "../actionmanager/command.h"
#include "../coreplugintr.h"
#include "../icore.h"

#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QFont>
#include <QLabel>
#include <QScrollArea>
#include <QStackedWidget>
#include <QToolButton>

static const char SETTINGSKEYSECTIONNAME[] = "SearchResults";
static const char SETTINGSKEYEXPANDRESULTS[] = "ExpandResults";

// Note that this is a soft limit: If all searches are still running, none of them will be
// removed when a new one is started.
static const int MAX_SEARCH_HISTORY = 12;

namespace Core {

/*!
    \namespace Core::Search
    \inmodule QtCreator
    \internal
*/

namespace Internal {

    class InternalScrollArea : public QScrollArea
    {
        Q_OBJECT
    public:
        explicit InternalScrollArea(QWidget *parent)
            : QScrollArea(parent)
        {
            setFrameStyle(QFrame::NoFrame);
            setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }

        QSize sizeHint() const override
        {
            if (widget())
                return widget()->size();
            return QScrollArea::sizeHint();
        }
    };

    class SearchResultWindowPrivate : public QObject
    {
    public:
        SearchResultWindowPrivate(SearchResultWindow *window, QWidget *newSearchPanel);
        bool isSearchVisible() const { return m_currentIndex > 0; }
        int visibleSearchIndex() const { return m_currentIndex - 1; }
        void setCurrentIndex(int index, bool focus);
        void setCurrentIndexWithFocus(int index) { setCurrentIndex(index, true); }
        void moveWidgetToTop(SearchResultWidget *widget);
        void popupRequested(SearchResultWidget *widget, bool focus);
        void handleExpandCollapseToolButton(bool checked);
        void updateFilterButton();
        int indexOfSearchToEvict() const;
        QList<QWidget *> toolBarWidgets();

        SearchResultWindow *q;
        QList<Internal::SearchResultWidget *> m_searchResultWidgets;
        QToolButton *m_expandCollapseButton;
        QToolButton *m_filterButton;
        QToolButton *m_newSearchButton;
        QAction *m_expandCollapseAction;
        static const bool m_initiallyExpand;
        QWidget *m_spacer;
        QLabel *m_historyLabel = nullptr;
        QWidget *m_spacer2;
        QComboBox *m_recentSearchesBox = nullptr;
        QStackedWidget *m_widget;
        QList<SearchResult *> m_searchResults;
        int m_currentIndex;
        QFont m_font;
        Utils::SearchResultColors m_colors;
        int m_tabWidth;

    };

    const bool SearchResultWindowPrivate::m_initiallyExpand = false;

    SearchResultWindowPrivate::SearchResultWindowPrivate(SearchResultWindow *window, QWidget *nsp) :
        q(window),
        m_expandCollapseButton(nullptr),
        m_expandCollapseAction(new QAction(Tr::tr("Expand All"), window)),
        m_spacer(new QWidget),
        m_spacer2(new QWidget),
        m_widget(new QStackedWidget),
        m_currentIndex(0),
        m_tabWidth(8)
    {
        m_spacer->setMinimumWidth(30);
        m_spacer2->setMinimumWidth(5);

        m_widget->setWindowTitle(q->displayName());

        auto newSearchArea = new InternalScrollArea(m_widget);
        newSearchArea->setWidget(nsp);
        newSearchArea->setFocusProxy(nsp);
        m_widget->addWidget(newSearchArea);

        m_expandCollapseButton = new QToolButton(m_widget);

        m_expandCollapseAction->setCheckable(true);
        m_expandCollapseAction->setIcon(Utils::Icons::EXPAND_ALL_TOOLBAR.icon());
        m_expandCollapseAction->setEnabled(false);
        Command *cmd = ActionManager::registerAction(m_expandCollapseAction, "Find.ExpandAll");
        cmd->setAttribute(Command::CA_UpdateText);
        m_expandCollapseButton->setDefaultAction(cmd->action());

        m_filterButton = new QToolButton(m_widget);
        m_filterButton->setText(Tr::tr("Filter Results"));
        m_filterButton->setIcon(Utils::Icons::FILTER.icon());
        m_filterButton->setEnabled(false);

        QAction *newSearchAction = new QAction(Tr::tr("New Search"), this);
        newSearchAction->setIcon(Utils::Icons::NEWSEARCH_TOOLBAR.icon());
        cmd = ActionManager::command(Constants::ADVANCED_FIND);
        m_newSearchButton = Command::toolButtonWithAppendedShortcut(newSearchAction, cmd);
        if (QTC_GUARD(cmd && cmd->action()))
            connect(m_newSearchButton, &QToolButton::triggered, cmd->action(), &QAction::trigger);

        connect(m_expandCollapseAction, &QAction::toggled,
                this, &SearchResultWindowPrivate::handleExpandCollapseToolButton);

        connect(m_filterButton, &QToolButton::clicked, this, [this] {
            if (!isSearchVisible())
                return;
            m_searchResultWidgets.at(visibleSearchIndex())->showFilterWidget(m_filterButton);
        });
    }

    void SearchResultWindowPrivate::setCurrentIndex(int index, bool focus)
    {
        QTC_ASSERT(m_recentSearchesBox, return );
        if (isSearchVisible())
            m_searchResultWidgets.at(visibleSearchIndex())->notifyVisibilityChanged(false);
        m_currentIndex = index;
        m_widget->setCurrentIndex(index);
        m_recentSearchesBox->setCurrentIndex(index);
        if (!isSearchVisible()) {
            if (focus)
                m_widget->currentWidget()->setFocus();
            m_expandCollapseAction->setEnabled(false);
            m_newSearchButton->setEnabled(false);
        } else {
            if (focus)
                m_searchResultWidgets.at(visibleSearchIndex())->setFocusInternally();
            m_searchResultWidgets.at(visibleSearchIndex())->notifyVisibilityChanged(true);
            m_expandCollapseAction->setEnabled(true);
            m_newSearchButton->setEnabled(true);
        }
        q->navigateStateChanged();
        updateFilterButton();
    }

    void SearchResultWindowPrivate::moveWidgetToTop(SearchResultWidget *widget)
    {
        QTC_ASSERT(m_recentSearchesBox, return);
        const int index = m_searchResultWidgets.indexOf(widget);
        if (index == 0)
            return; // nothing to do
        const int internalIndex = index + 1/*account for "new search" entry*/;
        QString searchEntry = m_recentSearchesBox->itemText(internalIndex);

        m_searchResultWidgets.removeAt(index);
        m_widget->removeWidget(widget);
        m_recentSearchesBox->removeItem(internalIndex);
        SearchResult *result = m_searchResults.takeAt(index);

        m_searchResultWidgets.prepend(widget);
        m_widget->insertWidget(1, widget);
        m_recentSearchesBox->insertItem(1, searchEntry);
        m_searchResults.prepend(result);

        // adapt the current index
        if (index == visibleSearchIndex()) {
            // was visible, so we switch
            // this is the default case
            m_currentIndex = 1;
            m_widget->setCurrentIndex(1);
            m_recentSearchesBox->setCurrentIndex(1);
        } else if (visibleSearchIndex() < index) {
            // academical case where the widget moved before the current widget
            // only our internal book keeping needed
            ++m_currentIndex;
        }
    }

    void SearchResultWindowPrivate::popupRequested(SearchResultWidget *widget, bool focus)
    {
        int internalIndex = m_searchResultWidgets.indexOf(widget) + 1/*account for "new search" entry*/;
        setCurrentIndex(internalIndex, focus);
        q->popup(focus ? IOutputPane::ModeSwitch | IOutputPane::WithFocus
                       : IOutputPane::NoModeSwitch);
    }
} // namespace Internal

using namespace Core::Internal;

/*!
    \enum Core::SearchResultWindow::SearchMode
    This enum type specifies whether a search should show the replace UI or not:

    \value SearchOnly
           The search does not support replace.
    \value SearchAndReplace
           The search supports replace, so show the UI for it.
*/

/*!
    \class Core::SearchResult
    \inheaderfile coreplugin/find/searchresultwindow.h
    \inmodule QtCreator

    \brief The SearchResult class reports user interaction, such as the
    activation of a search result item.

    Whenever a new search is initiated via startNewSearch, an instance of this
    class is returned to provide the initiator with the hooks for handling user
    interaction.
*/

/*!
    \fn void Core::SearchResult::activated(const Utils::SearchResultItem &item)
    Indicates that the user activated the search result \a item by
    double-clicking it, for example.
*/

/*!
    \fn void Core::SearchResult::replaceButtonClicked(const QString &replaceText,
                           const Utils::SearchResultItems &checkedItems,
                           bool preserveCase)

    Indicates that the user initiated a text replace by selecting
    \uicontrol {Replace All}, for example.

    The signal reports the text to use for replacement in \a replaceText,
    the list of search result items that were selected by the user
    in \a checkedItems, and whether a search and replace should preserve the
    case of the replaced strings in \a preserveCase.
    The handler of this signal should apply the replace only on the selected
    items.
*/

/*!
    \enum Core::SearchResult::AddMode
    This enum type specifies whether the search results should be sorted or
    ordered:

    \value AddSorted
           The search results are sorted.
    \value AddOrdered
           The search results are ordered.
*/

/*!
    \fn void Core::SearchResult::canceled()
    This signal is emitted if the user cancels the search.
*/

/*!
    \fn void Core::SearchResult::countChanged(int count)
    This signal is emitted when the number of search hits changes to \a count.
*/

/*!
    \fn void Core::SearchResult::paused(bool paused)
    This signal is emitted when the search status is set to \a paused.
*/

/*!
    \fn void Core::SearchResult::searchAgainRequested()

    This signal is emitted when the \uicontrol {Search Again} button is
    selected.
*/

/*!
    \fn void Core::SearchResult::visibilityChanged(bool visible)

    This signal is emitted when the visibility of the search results changes
    to \a visible.
*/

/*!
    \class Core::SearchResultWindow
    \inheaderfile coreplugin/find/searchresultwindow.h
    \inmodule QtCreator

    \brief The SearchResultWindow class is the implementation of a commonly
    shared \uicontrol{Search Results} output pane.

    \image qtcreator-search-results.webp {Search Results view}

    Whenever you want to show the user a list of search results, or want
    to present UI for a global search and replace, use the single instance
    of this class.

    In addition to being an implementation of an output pane, the
    SearchResultWindow has functions and enums that enable other
    plugins to show their search results and hook into the user actions for
    selecting an entry and performing a global replace.

    Whenever you start a search, call startNewSearch(SearchMode) to initialize
    the \uicontrol {Search Results} output pane. The parameter determines if the GUI for
    replacing should be shown.
    The function returns a SearchResult object that is your
    hook into the signals from user interaction for this search.
    When you produce search results, call addResults() or addResult() to add them
    to the \uicontrol {Search Results} output pane.
    After the search has finished call finishSearch() to inform the
    \uicontrol {Search Results} output pane about it.

    You will get activated() signals via your SearchResult instance when
    the user selects a search result item. If you started the search
    with the SearchAndReplace option, the replaceButtonClicked() signal
    is emitted when the user requests a replace.
*/

/*!
    \fn QString Core::SearchResultWindow::displayName() const
    \internal
*/

/*!
    \enum Core::SearchResultWindow::PreserveCaseMode
    This enum type specifies whether a search and replace should preserve the
    case of the replaced strings:

    \value PreserveCaseEnabled
           The case is preserved when replacings strings.
    \value PreserveCaseDisabled
           The given case is used when replacing strings.
*/

SearchResultWindow *SearchResultWindow::m_instance = nullptr;

/*!
    \internal
*/
SearchResultWindow::SearchResultWindow(QWidget *newSearchPanel)
    : d(new SearchResultWindowPrivate(this, newSearchPanel))
{
    setId("SearchResults");
    setDisplayName(Tr::tr("Search Results"));
    setPriorityInStatusBar(80);
    m_instance = this;
    readSettings();
}

/*!
    \internal
*/
SearchResultWindow::~SearchResultWindow()
{
    qDeleteAll(d->m_searchResults);
    delete d->m_widget;
    d->m_widget = nullptr;
    delete d;
}

/*!
    Returns the single shared instance of the \uicontrol {Search Results} output pane.
*/
SearchResultWindow *SearchResultWindow::instance()
{
    return m_instance;
}

/*!
    \internal
*/
void SearchResultWindow::visibilityChanged(bool visible)
{
    if (d->isSearchVisible())
        d->m_searchResultWidgets.at(d->visibleSearchIndex())->notifyVisibilityChanged(visible);
}

/*!
    \internal
*/
QWidget *SearchResultWindow::outputWidget(QWidget *)
{
    return d->m_widget;
}

/*!
    \internal
*/
QList<QWidget*> SearchResultWindow::toolBarWidgets() const
{
    return d->toolBarWidgets();
}

/*!
    Tells the \uicontrol {Search Results} output pane to start a new search.

    The \a label should be a string that shortly describes the type of the
    search, that is, the search filter and possibly the most relevant search
    option, followed by a colon (:). For example: \c {Project 'myproject':}
    The \a searchTerm is shown after the colon.

    The \a toolTip should elaborate on the search parameters, like file patterns
    that are searched and find flags.

    If \a cfgGroup is not empty, it will be used for storing the \e {do not ask again}
    setting of a \e {this change cannot be undone} warning (which is implicitly requested
    by passing a non-empty group).

    The \a searchOrSearchAndReplace parameter holds whether the search
    results pane should show a UI for a global search and replace action.
    The \a preserveCaseMode parameter holds whether the case of the search
    string should be preserved when replacing strings.

    Returns a SearchResult object that is used for signaling user interaction
    with the results of this search.
    The search result window owns the returned SearchResult
    and might delete it any time, even while the search is running.
    For example, when the user clears the \uicontrol {Search Results} pane, or when
    the user opens so many other searches that this search falls out of the history.

*/
SearchResult *SearchResultWindow::startNewSearch(const QString &label,
                                                 const QString &toolTip,
                                                 const QString &searchTerm,
                                                 SearchMode searchOrSearchAndReplace,
                                                 PreserveCaseMode preserveCaseMode,
                                                 const QString &cfgGroup)
{
    if (QTC_GUARD(d->m_recentSearchesBox)) {
        if (d->m_searchResults.size() >= MAX_SEARCH_HISTORY) {
            if (d->m_currentIndex >= d->m_recentSearchesBox->count() - 1) {
                // temporarily set the index to the last but one existing
                d->m_currentIndex = d->m_recentSearchesBox->count() - 2;
            }
            if (const int toRemoveIndex = d->indexOfSearchToEvict(); toRemoveIndex != -1) {
                SearchResultWidget * const widgetToRemove
                    = d->m_searchResultWidgets.takeAt(toRemoveIndex);
                widgetToRemove->notifyVisibilityChanged(false);
                // widget first, because that might send interesting signals to SearchResult
                delete widgetToRemove;
                delete d->m_searchResults.takeAt(toRemoveIndex);
                d->m_recentSearchesBox->removeItem(toRemoveIndex + 1);
            }
        }
        d->m_recentSearchesBox->insertItem(1, Tr::tr("%1 %2").arg(label, searchTerm));
    }
    auto widget = new SearchResultWidget;
    connect(widget, &SearchResultWidget::filterInvalidated, this, [this, widget] {
        if (widget == d->m_searchResultWidgets.at(d->visibleSearchIndex()))
            d->handleExpandCollapseToolButton(d->m_expandCollapseButton->isChecked());
    });
    connect(widget, &SearchResultWidget::filterChanged,
            d, &SearchResultWindowPrivate::updateFilterButton);
    d->m_searchResultWidgets.prepend(widget);
    d->m_widget->insertWidget(1, widget);
    connect(widget, &SearchResultWidget::navigateStateChanged,
            this, &SearchResultWindow::navigateStateChanged);
    connect(widget, &SearchResultWidget::restarted, d,
            [this, widget] { d->moveWidgetToTop(widget); });
    connect(widget, &SearchResultWidget::requestPopup, d,
            [this, widget](bool focus) { d->popupRequested(widget, focus); });
    widget->setTextEditorFont(d->m_font, d->m_colors);
    widget->setTabWidth(d->m_tabWidth);
    widget->setSupportPreserveCase(preserveCaseMode == PreserveCaseEnabled);
    bool supportsReplace = searchOrSearchAndReplace != SearchOnly;
    widget->setSupportsReplace(supportsReplace, supportsReplace ? cfgGroup : QString());
    widget->setAutoExpandResults(d->m_expandCollapseAction->isChecked());
    widget->setInfo(label, toolTip, searchTerm);
    auto result = new SearchResult(widget);
    d->m_searchResults.prepend(result);
    if (d->m_currentIndex > 0)
        ++d->m_currentIndex; // so setCurrentIndex still knows about the right "currentIndex" and its widget
    d->setCurrentIndexWithFocus(1);
    return result;
}

/*!
    Clears the current contents of the \uicontrol {Search Results} output pane.
*/
void SearchResultWindow::clearContents()
{
    if (QTC_GUARD(d->m_recentSearchesBox)) {
        for (int i = d->m_recentSearchesBox->count() - 1; i > 0 /* don't want i==0 */; --i)
            d->m_recentSearchesBox->removeItem(i);
    }
    for (Internal::SearchResultWidget *widget : std::as_const(d->m_searchResultWidgets))
        widget->notifyVisibilityChanged(false);
    qDeleteAll(d->m_searchResultWidgets);
    d->m_searchResultWidgets.clear();
    qDeleteAll(d->m_searchResults);
    d->m_searchResults.clear();

    d->m_currentIndex = 0;
    d->m_widget->currentWidget()->setFocus();
    d->m_expandCollapseAction->setEnabled(false);
    navigateStateChanged();

    d->m_newSearchButton->setEnabled(false);
}

/*!
    \internal
*/
bool SearchResultWindow::hasFocus() const
{
    QWidget *widget = d->m_widget->focusWidget();
    if (!widget)
        return false;
    return widget->window()->focusWidget() == widget;
}

/*!
    \internal
*/
bool SearchResultWindow::canFocus() const
{
    if (d->isSearchVisible())
        return d->m_searchResultWidgets.at(d->visibleSearchIndex())->canFocusInternally();
    return true;
}

/*!
    \internal
*/
void SearchResultWindow::setFocus()
{
    if (!d->isSearchVisible())
        d->m_widget->currentWidget()->setFocus();
    else
        d->m_searchResultWidgets.at(d->visibleSearchIndex())->setFocusInternally();
}

/*!
    \internal
*/
void SearchResultWindow::setTextEditorFont(const QFont &font,
                                           const Utils::SearchResultColors &colors)
{
    d->m_font = font;
    d->m_colors = colors;
    for (Internal::SearchResultWidget *widget : std::as_const(d->m_searchResultWidgets))
        widget->setTextEditorFont(font, colors);
}

/*!
    Sets the \uicontrol {Search Results} tab width to \a tabWidth.
*/
void SearchResultWindow::setTabWidth(int tabWidth)
{
    d->m_tabWidth = tabWidth;
    for (Internal::SearchResultWidget *widget : std::as_const(d->m_searchResultWidgets))
        widget->setTabWidth(tabWidth);
}
/*!
    Opens a new search panel.
*/
void SearchResultWindow::openNewSearchPanel()
{
    d->setCurrentIndexWithFocus(0);
    popup(IOutputPane::ModeSwitch  | IOutputPane::WithFocus | IOutputPane::EnsureSizeHint);
}

void SearchResultWindowPrivate::handleExpandCollapseToolButton(bool checked)
{
    if (!isSearchVisible())
        return;
    m_searchResultWidgets.at(visibleSearchIndex())->setAutoExpandResults(checked);
    if (checked) {
        m_expandCollapseAction->setText(Tr::tr("Collapse All"));
        m_searchResultWidgets.at(visibleSearchIndex())->expandAll();
    } else {
        m_expandCollapseAction->setText(Tr::tr("Expand All"));
        m_searchResultWidgets.at(visibleSearchIndex())->collapseAll();
    }
}

void SearchResultWindowPrivate::updateFilterButton()
{
    m_filterButton->setEnabled(isSearchVisible()
                               && m_searchResultWidgets.at(visibleSearchIndex())->hasFilter());
}

int SearchResultWindowPrivate::indexOfSearchToEvict() const
{
    for (int i = m_searchResultWidgets.size() - 1; i >= 0; --i) {
        if (!m_searchResultWidgets.at(i)->isSearching())
            return i;
    }
    return -1;
}

QList<QWidget *> SearchResultWindowPrivate::toolBarWidgets()
{
    if (!m_historyLabel)
        m_historyLabel = new QLabel(Tr::tr("History:"));
    if (!m_recentSearchesBox) {
        m_recentSearchesBox = new QComboBox;
        m_recentSearchesBox->setProperty(Utils::StyleHelper::C_DRAW_LEFT_BORDER, true);
        m_recentSearchesBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_recentSearchesBox->addItem(Tr::tr("New Search"));
        connect(m_recentSearchesBox, &QComboBox::activated,
                this, &SearchResultWindowPrivate::setCurrentIndexWithFocus);
    }
    return {m_expandCollapseButton,
            m_filterButton,
            m_newSearchButton,
            m_spacer,
            m_historyLabel,
            m_spacer2,
            m_recentSearchesBox};
}

/*!
    \internal
*/
void SearchResultWindow::readSettings()
{
    Utils::QtcSettings *s = ICore::settings();
    s->beginGroup(SETTINGSKEYSECTIONNAME);
    d->m_expandCollapseAction->setChecked(s->value(SETTINGSKEYEXPANDRESULTS,
                                                   SearchResultWindowPrivate::m_initiallyExpand).toBool());
    s->endGroup();
}

/*!
    \internal
*/
void SearchResultWindow::writeSettings()
{
    Utils::QtcSettings *s = ICore::settings();
    s->beginGroup(SETTINGSKEYSECTIONNAME);
    s->setValueWithDefault(SETTINGSKEYEXPANDRESULTS,
                           d->m_expandCollapseAction->isChecked(),
                           SearchResultWindowPrivate::m_initiallyExpand);
    s->endGroup();
}

/*!
    \internal
*/
bool SearchResultWindow::canNext() const
{
    if (d->isSearchVisible())
        return d->m_searchResultWidgets.at(d->visibleSearchIndex())->count() > 0;
    return false;
}

/*!
    \internal
*/
bool SearchResultWindow::canPrevious() const
{
    return canNext();
}

/*!
    \internal
*/
void SearchResultWindow::goToNext()
{
    int index = d->m_widget->currentIndex();
    if (index != 0)
        d->m_searchResultWidgets.at(index-1)->goToNext();
}

/*!
    \internal
*/
void SearchResultWindow::goToPrev()
{
    int index = d->m_widget->currentIndex();
    if (index != 0)
        d->m_searchResultWidgets.at(index-1)->goToPrevious();
}

/*!
    \internal
*/
bool SearchResultWindow::canNavigate() const
{
    return true;
}

/*!
    \internal
*/
SearchResult::SearchResult(SearchResultWidget *widget)
    : m_widget(widget)
{
    connect(widget, &SearchResultWidget::activated, this, &SearchResult::activated);
    connect(widget, &SearchResultWidget::replaceButtonClicked,
            this, &SearchResult::replaceButtonClicked);
    connect(widget, &SearchResultWidget::replaceTextChanged,
            this, &SearchResult::replaceTextChanged);
    connect(widget, &SearchResultWidget::canceled, this, &SearchResult::canceled);
    connect(widget, &SearchResultWidget::paused, this, &SearchResult::paused);
    connect(widget, &SearchResultWidget::visibilityChanged,
            this, &SearchResult::visibilityChanged);
    connect(widget, &SearchResultWidget::searchAgainRequested,
            this, &SearchResult::searchAgainRequested);
}

/*!
    Attaches some random \a data to this search, that you can use later.

    \sa userData()
*/
void SearchResult::setUserData(const QVariant &data)
{
    m_userData = data;
}

/*!
    Returns the data that was attached to this search by calling
    setUserData().

    \sa setUserData()
*/
QVariant SearchResult::userData() const
{
    return m_userData;
}

bool SearchResult::supportsReplace() const
{
    return m_widget->supportsReplace();
}

/*!
    Returns the text that should replace the text in search results.
*/
QString SearchResult::textToReplace() const
{
    return m_widget->textToReplace();
}

/*!
    Returns the number of search hits.
*/
int SearchResult::count() const
{
    return m_widget->count();
}

/*!
    Sets whether the \uicontrol {Seach Again} button is enabled to \a supported.
*/
void SearchResult::setSearchAgainSupported(bool supported)
{
    m_widget->setSearchAgainSupported(supported);
}

/*!
    Returns a UI for a global search and replace action.
*/
QWidget *SearchResult::additionalReplaceWidget() const
{
    return m_widget->additionalReplaceWidget();
}

/*!
    Sets a \a widget as UI for a global search and replace action.
*/
void SearchResult::setAdditionalReplaceWidget(QWidget *widget)
{
    m_widget->setAdditionalReplaceWidget(widget);
}

/*!
    Adds a single result line to the \uicontrol {Search Results} output pane.

    \a {item}.mainRange() specifies the region from the beginning of the search term
    through its length that should be visually marked.
    \a {item}.path(), \a {item}.text() are shown on the result line.
    You can attach arbitrary \a {item}.userData() to the search result, which can
    be used, for example, when reacting to the signals of the search results
    for your search.

    \sa addResults()
*/
void SearchResult::addResult(const Utils::SearchResultItem &item)
{
    m_widget->addResults({item}, AddOrdered);
}

/*!
    Adds the search result \a items to the \uicontrol {Search Results} output
    pane using \a mode.

    \sa addResult()
*/
void SearchResult::addResults(const Utils::SearchResultItems &items, AddMode mode)
{
    m_widget->addResults(items, mode);
    emit countChanged(m_widget->count());
}

void SearchResult::setFilter(SearchResultFilter *filter)
{
    m_widget->setFilter(filter);
}

/*!
    Notifies the \uicontrol {Search Results} output pane that the current search
    has been \a canceled for the specified \a reason, and the UI should reflect
    that.
*/
void SearchResult::finishSearch(bool canceled, const QString &reason)
{
    m_widget->finishSearch(canceled, reason);
    if (m_finishedHandler) {
        if (!canceled)
            m_widget->triggerReplace();
        m_finishedHandler();
        m_finishedHandler = {};
    }
}

/*!
    Sets the value in the UI element that allows the user to type
    the text that should replace text in search results to \a textToReplace.
*/
void SearchResult::setTextToReplace(const QString &textToReplace)
{
    m_widget->setTextToReplace(textToReplace);
}

/*!
    Sets whether replace is \a enabled and can be triggered by the user.
*/
void SearchResult::setReplaceEnabled(bool enabled)
{
    m_widget->setReplaceEnabled(enabled);
}

/*!
 * Removes all search results.
 */
void SearchResult::restart()
{
    m_widget->restart();
}

/*!
    Sets whether the \uicontrol {Seach Again} button is enabled to \a enabled.
*/
void SearchResult::setSearchAgainEnabled(bool enabled)
{
    m_widget->setSearchAgainEnabled(enabled);
}

/*!
 * Opens the \uicontrol {Search Results} output pane with this search.
 */
void SearchResult::popup()
{
    m_widget->sendRequestPopup();
}

void Core::SearchResult::makeNonInteractive(const std::function<void ()> &callback)
{
    QTC_ASSERT(callback, return);
    m_widget->setEnabled(false);
    m_finishedHandler = callback;
}

Utils::SearchResultItems SearchResult::allItems() const
{
    return m_widget->items(false);
}

} // namespace Core

#include "searchresultwindow.moc"
