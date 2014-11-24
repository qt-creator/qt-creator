/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "searchresultwindow.h"
#include "searchresultwidget.h"
#include "searchresultcolor.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>

#include <QAction>
#include <QComboBox>
#include <QDebug>
#include <QFont>
#include <QLabel>
#include <QScrollArea>
#include <QSettings>
#include <QStackedWidget>
#include <QToolButton>

static const char SETTINGSKEYSECTIONNAME[] = "SearchResults";
static const char SETTINGSKEYEXPANDRESULTS[] = "ExpandResults";
static const int MAX_SEARCH_HISTORY = 12;

namespace Core {

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

        QSize sizeHint() const
        {
            if (widget())
                return widget()->size();
            return QScrollArea::sizeHint();
        }
    };

    class SearchResultWindowPrivate : public QObject
    {
        Q_OBJECT
    public:
        SearchResultWindowPrivate(SearchResultWindow *window);
        bool isSearchVisible() const;
        int visibleSearchIndex() const;
        void setCurrentIndex(int index, bool focus);

        FindPlugin *m_plugin;
        SearchResultWindow *q;
        QList<Internal::SearchResultWidget *> m_searchResultWidgets;
        QToolButton *m_expandCollapseButton;
        QAction *m_expandCollapseAction;
        static const bool m_initiallyExpand = false;
        QWidget *m_spacer;
        QLabel *m_historyLabel;
        QWidget *m_spacer2;
        QComboBox *m_recentSearchesBox;
        QStackedWidget *m_widget;
        QList<SearchResult *> m_searchResults;
        int m_currentIndex;
        QFont m_font;
        SearchResultColor m_color;
        int m_tabWidth;

    public slots:
        void setCurrentIndex(int index);
        void moveWidgetToTop();
        void popupRequested(bool focus);
    };

    SearchResultWindowPrivate::SearchResultWindowPrivate(SearchResultWindow *window)
        : q(window),
          m_tabWidth(8)
    {
    }

    bool SearchResultWindowPrivate::isSearchVisible() const
    {
        return m_currentIndex > 0;
    }

    int SearchResultWindowPrivate::visibleSearchIndex() const
    {
        return m_currentIndex - 1;
    }

    void SearchResultWindowPrivate::setCurrentIndex(int index, bool focus)
    {
        if (isSearchVisible())
            m_searchResultWidgets.at(visibleSearchIndex())->notifyVisibilityChanged(false);
        m_currentIndex = index;
        m_widget->setCurrentIndex(index);
        m_recentSearchesBox->setCurrentIndex(index);
        if (!isSearchVisible()) {
            if (focus)
                m_widget->currentWidget()->setFocus();
            m_expandCollapseButton->setEnabled(false);
        } else {
            if (focus)
                m_searchResultWidgets.at(visibleSearchIndex())->setFocusInternally();
            m_searchResultWidgets.at(visibleSearchIndex())->notifyVisibilityChanged(true);
            m_expandCollapseButton->setEnabled(true);
        }
        q->navigateStateChanged();
    }

    void SearchResultWindowPrivate::setCurrentIndex(int index)
    {
        setCurrentIndex(index, true/*focus*/);
    }

    void SearchResultWindowPrivate::moveWidgetToTop()
    {
        SearchResultWidget *widget = qobject_cast<SearchResultWidget *>(sender());
        QTC_ASSERT(widget, return);
        int index = m_searchResultWidgets.indexOf(widget);
        if (index == 0)
            return; // nothing to do
        int internalIndex = index + 1/*account for "new search" entry*/;
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

    void SearchResultWindowPrivate::popupRequested(bool focus)
    {
        SearchResultWidget *widget = qobject_cast<SearchResultWidget *>(sender());
        QTC_ASSERT(widget, return);
        int internalIndex = m_searchResultWidgets.indexOf(widget) + 1/*account for "new search" entry*/;
        setCurrentIndex(internalIndex, focus);
        q->popup(focus ? Core::IOutputPane::ModeSwitch | Core::IOutputPane::WithFocus
                       : Core::IOutputPane::NoModeSwitch);
    }
}

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
    \brief The SearchResult class reports user interaction, such as the
    activation of a search result item.

    Whenever a new search is initiated via startNewSearch, an instance of this
    class is returned to provide the initiator with the hooks for handling user
    interaction.
*/

/*!
    \fn void SearchResult::activated(const Core::SearchResultItem &item)
    Indicates that the user activated the search result \a item by
    double-clicking it, for example.
*/

/*!
    \fn void SearchResult::replaceButtonClicked(const QString &replaceText, const QList<Core::SearchResultItem> &checkedItems, bool preserveCase)
    Indicates that the user initiated a text replace by selecting
    \gui {Replace All}, for example.

    The signal reports the text to use for replacement in \a replaceText,
    and the list of search result items that were selected by the user
    in \a checkedItems.
    The handler of this signal should apply the replace only on the selected
    items.
*/

/*!
    \class Core::SearchResultWindow
    \brief The SearchResultWindow class is the implementation of a commonly
    shared \gui{Search Results} output pane. Use it to show search results
    to a user.

    Whenever you want to show the user a list of search results, or want
    to present UI for a global search and replace, use the single instance
    of this class.

    Except for being an implementation of a output pane, the
    SearchResultWindow has a few functions and one enum that allows other
    plugins to show their search results and hook into the user actions for
    selecting an entry and performing a global replace.

    Whenever you start a search, call startNewSearch(SearchMode) to initialize
    the \gui {Search Results} output pane. The parameter determines if the GUI for
    replacing should be shown.
    The function returns a SearchResult object that is your
    hook into the signals from user interaction for this search.
    When you produce search results, call addResults or addResult to add them
    to the \gui {Search Results} output pane.
    After the search has finished call finishSearch to inform the
    \gui {Search Results} output pane about it.

    You will get activated signals via your SearchResult instance when
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
    \internal
*/
SearchResultWindow::SearchResultWindow(QWidget *newSearchPanel)
    : d(new SearchResultWindowPrivate(this))
{
    m_instance = this;

    d->m_spacer = new QWidget;
    d->m_spacer->setMinimumWidth(30);
    d->m_historyLabel = new QLabel(tr("History:"));
    d->m_spacer2 = new QWidget;
    d->m_spacer2->setMinimumWidth(5);
    d->m_recentSearchesBox = new QComboBox;
    d->m_recentSearchesBox->setProperty("drawleftborder", true);
    d->m_recentSearchesBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    d->m_recentSearchesBox->addItem(tr("New Search"));
    connect(d->m_recentSearchesBox, SIGNAL(activated(int)), d, SLOT(setCurrentIndex(int)));

    d->m_widget = new QStackedWidget;
    d->m_widget->setWindowTitle(displayName());

    InternalScrollArea *newSearchArea = new InternalScrollArea(d->m_widget);
    newSearchArea->setWidget(newSearchPanel);
    newSearchArea->setFocusProxy(newSearchPanel);
    d->m_widget->addWidget(newSearchArea);
    d->m_currentIndex = 0;

    d->m_expandCollapseButton = new QToolButton(d->m_widget);
    d->m_expandCollapseButton->setAutoRaise(true);

    d->m_expandCollapseAction = new QAction(tr("Expand All"), this);
    d->m_expandCollapseAction->setCheckable(true);
    d->m_expandCollapseAction->setIcon(QIcon(QLatin1String(":/find/images/expand.png")));
    Core::Command *cmd = Core::ActionManager::registerAction(
            d->m_expandCollapseAction, "Find.ExpandAll",
            Core::Context(Core::Constants::C_GLOBAL));
    cmd->setAttribute(Core::Command::CA_UpdateText);
    d->m_expandCollapseButton->setDefaultAction(cmd->action());

    connect(d->m_expandCollapseAction, SIGNAL(toggled(bool)), this, SLOT(handleExpandCollapseToolButton(bool)));
    readSettings();
}

/*!
    \internal
*/
SearchResultWindow::~SearchResultWindow()
{
    qDeleteAll(d->m_searchResults);
    delete d->m_widget;
    d->m_widget = 0;
    delete d;
}

/*!
    Returns the single shared instance of the \gui {Search Results} output pane.
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
    return QList<QWidget*>() << d->m_expandCollapseButton << d->m_spacer
                             << d->m_historyLabel << d->m_spacer2 << d->m_recentSearchesBox;
}

/*!
    Tells the \gui {Search Results} output pane to start a new search.

    The \a label should be a string that shortly describes the type of the
    search, that is, the search filter and possibly the most relevant search
     option, followed by a colon ':'. For example: \c {Project 'myproject':}
    The \a searchTerm is shown after the colon.
    The \a toolTip should elaborate on the search parameters, like file patterns that are searched and
    find flags.
    If \a cfgGroup is not empty, it will be used for storing the "do not ask again"
    setting of a "this change cannot be undone" warning (which is implicitly requested
    by passing a non-empty group).
    Returns a SearchResult object that is used for signaling user interaction
    with the results of this search.
    The search result window owns the returned SearchResult
    and might delete it any time, even while the search is running
    (for example, when the user clears the \gui {Search Results} pane, or when
    the user opens so many other searches
    that this search falls out of the history).

*/
SearchResult *SearchResultWindow::startNewSearch(const QString &label,
                                                 const QString &toolTip,
                                                 const QString &searchTerm,
                                                 SearchMode searchOrSearchAndReplace,
                                                 PreserveCaseMode preserveCaseMode,
                                                 const QString &cfgGroup)
{
    if (d->m_searchResults.size() >= MAX_SEARCH_HISTORY) {
        d->m_searchResultWidgets.last()->notifyVisibilityChanged(false);
        // widget first, because that might send interesting signals to SearchResult
        delete d->m_searchResultWidgets.takeLast();
        delete d->m_searchResults.takeLast();
        d->m_recentSearchesBox->removeItem(d->m_recentSearchesBox->count()-1);
        if (d->m_currentIndex >= d->m_recentSearchesBox->count()) {
            // temporarily set the index to the last existing
            d->m_currentIndex = d->m_recentSearchesBox->count() - 1;
        }
    }
    Internal::SearchResultWidget *widget = new Internal::SearchResultWidget;
    d->m_searchResultWidgets.prepend(widget);
    d->m_widget->insertWidget(1, widget);
    connect(widget, SIGNAL(navigateStateChanged()), this, SLOT(navigateStateChanged()));
    connect(widget, SIGNAL(restarted()), d, SLOT(moveWidgetToTop()));
    connect(widget, SIGNAL(requestPopup(bool)), d, SLOT(popupRequested(bool)));
    widget->setTextEditorFont(d->m_font, d->m_color);
    widget->setTabWidth(d->m_tabWidth);
    widget->setSupportPreserveCase(preserveCaseMode == PreserveCaseEnabled);
    widget->setShowReplaceUI(searchOrSearchAndReplace != SearchOnly);
    widget->setAutoExpandResults(d->m_expandCollapseAction->isChecked());
    widget->setInfo(label, toolTip, searchTerm);
    if (searchOrSearchAndReplace == SearchAndReplace)
        widget->setDontAskAgainGroup(cfgGroup);
    SearchResult *result = new SearchResult(widget);
    d->m_searchResults.prepend(result);
    d->m_recentSearchesBox->insertItem(1, tr("%1 %2").arg(label, searchTerm));
    if (d->m_currentIndex > 0)
        ++d->m_currentIndex; // so setCurrentIndex still knows about the right "currentIndex" and its widget
    d->setCurrentIndex(1);
    return result;
}

/*!
    Clears the current contents of the \gui {Search Results} output pane.
*/
void SearchResultWindow::clearContents()
{
    for (int i = d->m_recentSearchesBox->count() - 1; i > 0 /* don't want i==0 */; --i)
        d->m_recentSearchesBox->removeItem(i);
    foreach (Internal::SearchResultWidget *widget, d->m_searchResultWidgets)
        widget->notifyVisibilityChanged(false);
    qDeleteAll(d->m_searchResultWidgets);
    d->m_searchResultWidgets.clear();
    qDeleteAll(d->m_searchResults);
    d->m_searchResults.clear();

    d->m_currentIndex = 0;
    d->m_widget->currentWidget()->setFocus();
    d->m_expandCollapseButton->setEnabled(false);
    navigateStateChanged();
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
                                           const QColor &textForegroundColor,
                                           const QColor &textBackgroundColor,
                                           const QColor &highlightForegroundColor,
                                           const QColor &highlightBackgroundColor)
{
    d->m_font = font;
    Internal::SearchResultColor color;
    color.textBackground = textBackgroundColor;
    color.textForeground = textForegroundColor;
    color.highlightBackground = highlightBackgroundColor.isValid()
            ? highlightBackgroundColor
            : textBackgroundColor;
    color.highlightForeground = highlightForegroundColor.isValid()
            ? highlightForegroundColor
            : textForegroundColor;
    d->m_color = color;
    foreach (Internal::SearchResultWidget *widget, d->m_searchResultWidgets)
        widget->setTextEditorFont(font, color);
}

void SearchResultWindow::setTabWidth(int tabWidth)
{
    d->m_tabWidth = tabWidth;
    foreach (Internal::SearchResultWidget *widget, d->m_searchResultWidgets)
        widget->setTabWidth(tabWidth);
}

void SearchResultWindow::openNewSearchPanel()
{
    d->setCurrentIndex(0);
    popup(IOutputPane::ModeSwitch  | IOutputPane::WithFocus | IOutputPane::EnsureSizeHint);
}

/*!
    \internal
*/
void SearchResultWindow::handleExpandCollapseToolButton(bool checked)
{
    if (!d->isSearchVisible())
        return;
    d->m_searchResultWidgets.at(d->visibleSearchIndex())->setAutoExpandResults(checked);
    if (checked) {
        d->m_expandCollapseAction->setText(tr("Collapse All"));
        d->m_searchResultWidgets.at(d->visibleSearchIndex())->expandAll();
    } else {
        d->m_expandCollapseAction->setText(tr("Expand All"));
        d->m_searchResultWidgets.at(d->visibleSearchIndex())->collapseAll();
    }
}

/*!
    \internal
*/
void SearchResultWindow::readSettings()
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(SETTINGSKEYSECTIONNAME));
    d->m_expandCollapseAction->setChecked(s->value(QLatin1String(SETTINGSKEYEXPANDRESULTS), d->m_initiallyExpand).toBool());
    s->endGroup();
}

/*!
    \internal
*/
void SearchResultWindow::writeSettings()
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(SETTINGSKEYSECTIONNAME));
    s->setValue(QLatin1String(SETTINGSKEYEXPANDRESULTS), d->m_expandCollapseAction->isChecked());
    s->endGroup();
}

/*!
    \internal
*/
int SearchResultWindow::priorityInStatusBar() const
{
    return 80;
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
    connect(widget, SIGNAL(activated(Core::SearchResultItem)),
            this, SIGNAL(activated(Core::SearchResultItem)));
    connect(widget, SIGNAL(replaceButtonClicked(QString,QList<Core::SearchResultItem>,bool)),
            this, SIGNAL(replaceButtonClicked(QString,QList<Core::SearchResultItem>,bool)));
    connect(widget, SIGNAL(cancelled()),
            this, SIGNAL(cancelled()));
    connect(widget, SIGNAL(paused(bool)),
            this, SIGNAL(paused(bool)));
    connect(widget, SIGNAL(visibilityChanged(bool)),
            this, SIGNAL(visibilityChanged(bool)));
    connect(widget, SIGNAL(searchAgainRequested()),
            this, SIGNAL(searchAgainRequested()));
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

/*!
    Returns the text that should replace the text in search results.
*/
QString SearchResult::textToReplace() const
{
    return m_widget->textToReplace();
}

int SearchResult::count() const
{
    return m_widget->count();
}

void SearchResult::setSearchAgainSupported(bool supported)
{
    m_widget->setSearchAgainSupported(supported);
}

/*!
    Adds a single result line to the \gui {Search Results} output pane.

    \a fileName, \a lineNumber, and \a lineText are shown on the result line.
    \a searchTermStart and \a searchTermLength specify the region that
    should be visually marked (string position and length in \a lineText).
    You can attach arbitrary \a userData to the search result, which can
    be used, for example, when reacting to the signals of the search results
    for your search.

    \sa addResults()
*/
void SearchResult::addResult(const QString &fileName, int lineNumber, const QString &lineText,
                             int searchTermStart, int searchTermLength, const QVariant &userData)
{
    m_widget->addResult(fileName, lineNumber, lineText,
                        searchTermStart, searchTermLength, userData);
    emit countChanged(m_widget->count());
}

/*!
    Adds the search result \a items to the \gui {Search Results} output pane.

    \sa addResult()
*/
void SearchResult::addResults(const QList<SearchResultItem> &items, AddMode mode)
{
    m_widget->addResults(items, mode);
    emit countChanged(m_widget->count());
}

/*!
    Notifies the \gui {Search Results} output pane that the current search
    has finished, and the UI should reflect that.
*/
void SearchResult::finishSearch(bool canceled)
{
    m_widget->finishSearch(canceled);
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
 * Removes all search results.
 */
void SearchResult::restart()
{
    m_widget->restart();
}

void SearchResult::setSearchAgainEnabled(bool enabled)
{
    m_widget->setSearchAgainEnabled(enabled);
}

/*!
 * Opens the \gui {Search Results} output pane with this search.
 */
void SearchResult::popup()
{
    m_widget->sendRequestPopup();
}

} // namespace Core

#include "searchresultwindow.moc"
