/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "searchresultwindow.h"
#include "searchresultwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <utils/qtcassert.h>

#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtGui/QVBoxLayout>
#include <QtGui/QFont>
#include <QtGui/QAction>

static const char SETTINGSKEYSECTIONNAME[] = "SearchResults";
static const char SETTINGSKEYEXPANDRESULTS[] = "ExpandResults";

namespace Find {

namespace Internal {

    struct SearchResultWindowPrivate {
        SearchResultWindowPrivate();

        Internal::SearchResultWidget *m_searchResultWidget;
        QToolButton *m_expandCollapseButton;
        QAction *m_expandCollapseAction;
        static const bool m_initiallyExpand = false;
        QWidget *m_widget;
        SearchResult *m_currentSearch;
    };

    SearchResultWindowPrivate::SearchResultWindowPrivate()
        : m_currentSearch(0)
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
    \fn SearchResultWindow::SearchResultWindow()
    \internal
*/
SearchResultWindow::SearchResultWindow() : d(new SearchResultWindowPrivate)
{
    m_instance = this;
    d->m_widget = new QWidget;
    d->m_widget->setWindowTitle(displayName());

    d->m_searchResultWidget = new Internal::SearchResultWidget(d->m_widget);

    QVBoxLayout *vlay = new QVBoxLayout;
    d->m_widget->setLayout(vlay);
    vlay->setMargin(0);
    vlay->setSpacing(0);
    vlay->addWidget(d->m_searchResultWidget);

    d->m_expandCollapseButton = new QToolButton(d->m_widget);
    d->m_expandCollapseButton->setAutoRaise(true);

    d->m_expandCollapseAction = new QAction(tr("Expand All"), this);
    d->m_expandCollapseAction->setCheckable(true);
    d->m_expandCollapseAction->setIcon(QIcon(QLatin1String(":/find/images/expand.png")));
    Core::Command *cmd = Core::ICore::instance()->actionManager()->registerAction(
            d->m_expandCollapseAction, "Find.ExpandAll",
            Core::Context(Core::Constants::C_GLOBAL));
    cmd->setAttribute(Core::Command::CA_UpdateText);
    d->m_expandCollapseButton->setDefaultAction(cmd->action());

    connect(d->m_expandCollapseAction, SIGNAL(toggled(bool)), this, SLOT(handleExpandCollapseToolButton(bool)));
    connect(d->m_searchResultWidget, SIGNAL(navigateStateChanged()), this, SLOT(navigateStateChanged()));
    readSettings();
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
    \fn void SearchResultWindow::visibilityChanged(bool)
    \internal
*/
void SearchResultWindow::visibilityChanged(bool visible)
{
    d->m_searchResultWidget->notifyVisibilityChanged(visible);
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
    return QList<QWidget*>() << d->m_expandCollapseButton << d->m_searchResultWidget->toolBarWidgets();
}

/*!
    \brief Tells the search results window to start a new search.

    This will clear the contents of the previous search and initialize the UI
    with regard to showing the replace UI or not (depending on the search mode
    in \a searchOrSearchAndReplace).
    If \a cfgGroup is not empty, it will be used for storing the "do not ask again"
    setting of a "this change cannot be undone" warning (which is implicitly requested
    by passing a non-empty group).
    Returns a SearchResult object that is used for signaling user interaction
    with the results of this search.
*/
SearchResult *SearchResultWindow::startNewSearch(SearchMode searchOrSearchAndReplace, const QString &cfgGroup)
{
    clearContents();
    d->m_searchResultWidget->setShowReplaceUI(searchOrSearchAndReplace != SearchOnly);
    d->m_searchResultWidget->setDontAskAgainGroup(cfgGroup);
    delete d->m_currentSearch;
    d->m_currentSearch = new SearchResult(d->m_searchResultWidget);
    return d->m_currentSearch;
}

/*!
    \fn void SearchResultWindow::clearContents()
    \brief Clears the current contents in the search result window.
*/
void SearchResultWindow::clearContents()
{
    d->m_searchResultWidget->clear();
    navigateStateChanged();
}

/*!
    \fn bool SearchResultWindow::hasFocus()
    \internal
*/
bool SearchResultWindow::hasFocus()
{
    return d->m_searchResultWidget->hasFocusInternally();
}

/*!
    \fn bool SearchResultWindow::canFocus()
    \internal
*/
bool SearchResultWindow::canFocus()
{
    return d->m_searchResultWidget->canFocusInternally();
}

/*!
    \fn void SearchResultWindow::setFocus()
    \internal
*/
void SearchResultWindow::setFocus()
{
    d->m_searchResultWidget->setFocusInternally();
}

/*!
    \fn void SearchResultWindow::setTextEditorFont(const QFont &font)
    \internal
*/
void SearchResultWindow::setTextEditorFont(const QFont &font)
{
    d->m_searchResultWidget->setTextEditorFont(font);
}

/*!
    \fn void SearchResultWindow::handleExpandCollapseToolButton(bool checked)
    \internal
*/
void SearchResultWindow::handleExpandCollapseToolButton(bool checked)
{
    d->m_searchResultWidget->setAutoExpandResults(checked);
    if (checked) {
        d->m_expandCollapseAction->setText(tr("Collapse All"));
        d->m_searchResultWidget->expandAll();
    } else {
        d->m_expandCollapseAction->setText(tr("Expand All"));
        d->m_searchResultWidget->collapseAll();
    }
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
    return d->m_searchResultWidget->count() > 0;
}

/*!
    \fn bool SearchResultWindow::canPrevious()
    \internal
*/
bool SearchResultWindow::canPrevious()
{
    return d->m_searchResultWidget->count() > 0;
}

/*!
    \fn void SearchResultWindow::goToNext()
    \internal
*/
void SearchResultWindow::goToNext()
{
    d->m_searchResultWidget->goToNext();
}

/*!
    \fn void SearchResultWindow::goToPrev()
    \internal
*/
void SearchResultWindow::goToPrev()
{
    d->m_searchResultWidget->goToPrevious();
}

/*!
    \fn bool SearchResultWindow::canNavigate()
    \internal
*/
bool SearchResultWindow::canNavigate()
{
    return true;
}

/*!
    \fn SearchResult::SearchResult(SearchResultWidget *widget)
    \internal
*/
SearchResult::SearchResult(SearchResultWidget *widget)
    : m_widget(widget)
{
    connect(widget, SIGNAL(activated(Find::SearchResultItem)),
            this, SIGNAL(activated(Find::SearchResultItem)));
    connect(widget, SIGNAL(replaceButtonClicked(QString,QList<Find::SearchResultItem>)),
            this, SIGNAL(replaceButtonClicked(QString,QList<Find::SearchResultItem>)));
    connect(widget, SIGNAL(visibilityChanged(bool)),
            this, SIGNAL(visibilityChanged(bool)));
}

/*!
    \fn void SearchResult::setUserData(const QVariant &data)
    \brief Attach some random \a data to this search, that you can use later.

    \sa userData()
*/
void SearchResult::setUserData(const QVariant &data)
{
    m_userData = data;
}

/*!
    \fn void SearchResult::userData()
    \brief Return the data that was attached to this search by calling setUserData().

    \sa setUserData()
*/
QVariant SearchResult::userData() const
{
    return m_userData;
}

/*!
    \fn QString SearchResult::textToReplace() const
    \brief Returns the text that should replace the text in search results.
*/
QString SearchResult::textToReplace() const
{
    return m_widget->textToReplace();
}

/*!
    \fn void SearchResult::addResult(const QString &fileName, int lineNumber, const QString &rowText, int searchTermStart, int searchTermLength, const QVariant &userData)
    \brief Adds a single result line to the search results.

    The \a fileName, \a lineNumber and \a rowText are shown in the result line.
    \a searchTermStart and \a searchTermLength specify the region that
    should be visually marked (string position and length in \a rowText).
    You can attach arbitrary \a userData to the search result, which can
    be used e.g. when reacting to the signals of the SearchResult for your search.

    \sa addResults()
*/
void SearchResult::addResult(const QString &fileName, int lineNumber, const QString &lineText,
                             int searchTermStart, int searchTermLength, const QVariant &userData)
{
    m_widget->addResult(fileName, lineNumber, lineText,
                        searchTermStart, searchTermLength, userData);
}

/*!
    \fn void SearchResult::addResults(const QList<SearchResultItem> &items, SearchResult::AddMode mode)
    \brief Adds all of the given search result \a items to the search
    results window.

    \sa addResult()
*/
void SearchResult::addResults(const QList<SearchResultItem> &items, AddMode mode)
{
    m_widget->addResults(items, mode);
}

/*!
    \fn void SearchResult::finishSearch()
    \brief Notifies the search result window that the current search
    has finished, and the UI should reflect that.
*/
void SearchResult::finishSearch()
{
    m_widget->finishSearch();
}

/*!
    \fn void SearchResult::setTextToReplace(const QString &textToReplace)
    \brief Sets the value in the UI element that allows the user to type
    the text that should replace text in search results to \a textToReplace.
*/
void SearchResult::setTextToReplace(const QString &textToReplace)
{
    m_widget->setTextToReplace(textToReplace);
}

} // namespace Find
