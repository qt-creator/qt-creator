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

#include "ifindfilter.h"

#include <coreplugin/coreicons.h>

#include <QApplication>
#include <QKeySequence>
#include <QPainter>
#include <QPixmap>

/*!
    \class Find::IFindFilter
    \brief The IFindFilter class is the base class for find implementations
    that are invoked by selecting \gui Edit > \gui {Find/Replace} >
    \gui {Advanced Find}.

    Implementations of this class add an additional \gui Scope to the \gui {Advanced
    Find} dialog. That can be any search that requires the user to provide
    a text based search term (potentially with find flags like
    searching case sensitively or using regular expressions). Existing
    scopes are \gui {All Projects} that searches from all files in all projects
    and \gui {Files in File System} where the user provides a directory and file
    patterns to search.

    To make your find scope available to the user, you need to implement this
    class, and register an instance of your subclass in the plugin manager.

    A common way to present the search results to the user, is to use the
    shared \gui{Search Results} panel.

    If you want to implement a find filter that is doing a file based text
    search, you should use Find::BaseFileFind, which already implements all
    the details for this kind of search, only requiring you to provide an
    iterator over the file names of the files that should be searched.

    If you want to implement a more specialized find filter, you need to:
    \list
        \li Start your search in a separate thread
        \li Make this known to the Core::ProgressManager, for a progress bar
           and the ability to cancel the search
        \li Interface with the shared \gui{Search Results} panel, to show
           the search results, handle the event that the user click on one
           of the search result items, and possible handle a global replace
           of all or some of the search result items.
    \endlist

    Luckily QtConcurrent and the search result panel provide the frameworks
    that make it relatively easy to implement,
    while ensuring a common way for the user.

    The common pattern is roughly this:

    Implement the actual search within a QtConcurrent based function, that is
    a function that takes a \c{QFutureInterface<MySearchResult> &future}
    as the first parameter and the other information needed for the search
    as additional parameters. It should set useful progress information
    on the QFutureInterface, regularly check for \c{future.isPaused()}
    and \c{future.isCanceled()}, and report the search results
    (possibly in chunks) via \c{future.reportResult}.

    In the find filter's find/replaceAll function, get the shared
    \gui{Search Results} window, initiate a new search and connect the
    signals for handling selection of results and the replace action
    (see the Core::SearchResultWindow class for details).
    Start your search implementation via the corresponding QtConcurrent
    functions. Add the returned QFuture object to the Core::ProgressManager.
    Use a QFutureWatcher on the returned QFuture object to receive a signal
    when your search implementation reports search results, and add these
    to the shared \gui{Search Results} window.
*/

/*!
    \fn IFindFilter::~IFindFilter()
    \internal
*/

/*!
    \fn QString IFindFilter::id() const
    Returns the unique string identifier for this find filter.

    Usually should be something like "MyPlugin.MyFindFilter".
*/

/*!
    \fn QString IFindFilter::displayName() const
    Returns the name of the find filter or scope as presented to the user.

    This is the name that appears in the scope selection combo box, for example.
    Always return a translatable string (that is, use tr() for the return value).
*/

/*!
    \fn bool IFindFilter::isEnabled() const
    Returns whether the user should be able to select this find filter
    at the moment.

    This is used for the \gui {Current Projects} scope, for example. If the user
    has not
    opened a project, the scope is disabled.

    \sa changed()
*/

/*!
    \fn QKeySequence IFindFilter::defaultShortcut() const
    Returns the shortcut that can be used to open the advanced find
    dialog with this filter or scope preselected.

    Usually return an empty shortcut here, the user can still choose and
    assign a specific shortcut to this find scope via the preferences.
*/

/*!
    \fn bool IFindFilter::isReplaceSupported() const
    Returns whether the find filter supports search and replace.

    The default value is false, override this function to return \c true, if
    your find filter supports global search and replace.
*/

/*!
    \fn void IFindFilter::findAll(const QString &txt, Core::FindFlags findFlags)
    This function is called when the user selected this find scope and
    initiated a search.

    You should start a thread which actually performs the search for \a txt
    using the given \a findFlags
    (add it to Core::ProgressManager for a progress bar!) and presents the
    search results to the user (using the \gui{Search Results} output pane).
    For more information, see the descriptions of this class,
    Core::ProgressManager, and Core::SearchResultWindow.

    \sa replaceAll()
    \sa Core::ProgressManager
    \sa Core::SearchResultWindow
*/

/*!
    \fn void IFindFilter::replaceAll(const QString &txt, Core::FindFlags findFlags)
    Override this function if you want to support search and replace.

    This function is called when the user selected this find scope and
    initiated a search and replace.
    The default implementation does nothing.

    You should start a thread which actually performs the search for \a txt
    using the given \a findFlags
    (add it to Core::ProgressManager for a progress bar!) and presents the
    search results to the user (using the \gui{Search Results} output pane).
    For more information see the descriptions of this class,
    Core::ProgressManager, and Core::SearchResultWindow.

    \sa findAll()
    \sa Core::ProgressManager
    \sa Core::SearchResultWindow
*/

/*!
    \fn QWidget *IFindFilter::createConfigWidget()
    Returns a widget that contains additional controls for options
    for this find filter.

    The widget will be shown below the common options in the \gui {Advanced Find}
    dialog. It will be reparented and deleted by the find plugin.
*/

/*!
    \fn void IFindFilter::writeSettings(QSettings *settings)
    Called at shutdown to write the state of the additional options
    for this find filter to the \a settings.
*/

/*!
    \fn void IFindFilter::readSettings(QSettings *settings)
    Called at startup to read the state of the additional options
    for this find filter from the \a settings.
*/

/*!
    \fn void IFindFilter::enabledChanged(bool enabled)

    Signals that the enabled state of this find filter has changed.
*/

/*!
    \fn Core::FindFlags BaseTextFind::supportedFindFlags() const
    Returns the find flags, like whole words or regular expressions,
    that this find filter supports.

    Depending on the returned value, the default find option widgets are
    enabled or disabled.
    The default is Find::FindCaseSensitively, Find::FindRegularExpression
    and Find::FindWholeWords
*/

namespace Core {

QKeySequence IFindFilter::defaultShortcut() const
{
    return QKeySequence();
}

FindFlags IFindFilter::supportedFindFlags() const
{
    return FindCaseSensitively
            | FindRegularExpression | FindWholeWords;
}

QPixmap IFindFilter::pixmapForFindFlags(FindFlags flags)
{
    static const QPixmap casesensitiveIcon = Icons::FIND_CASE_INSENSITIVELY.pixmap();
    static const QPixmap regexpIcon = Icons::FIND_REGEXP.pixmap();
    static const QPixmap wholewordsIcon = Icons::FIND_WHOLE_WORD.pixmap();
    static const QPixmap preservecaseIcon = Icons::FIND_PRESERVE_CASE.pixmap();
    bool casesensitive = flags & FindCaseSensitively;
    bool wholewords = flags & FindWholeWords;
    bool regexp = flags & FindRegularExpression;
    bool preservecase = flags & FindPreserveCase;
    int width = 0;
    if (casesensitive)
        width += casesensitiveIcon.width();
    if (wholewords)
        width += wholewordsIcon.width();
    if (regexp)
        width += regexpIcon.width();
    if (preservecase)
        width += preservecaseIcon.width();
    if (width == 0)
        return QPixmap();
    QPixmap pixmap(QSize(width, casesensitiveIcon.height()));
    pixmap.fill(QColor(0xff, 0xff, 0xff, 0x28)); // Subtile contrast for dark themes
    const int dpr = int(qApp->devicePixelRatio());
    pixmap.setDevicePixelRatio(dpr);
    QPainter painter(&pixmap);
    int x = 0;
    if (casesensitive) {
        painter.drawPixmap(x, 0, casesensitiveIcon);
        x += casesensitiveIcon.width() / dpr;
    }
    if (wholewords) {
        painter.drawPixmap(x, 0, wholewordsIcon);
        x += wholewordsIcon.width() / dpr;
    }
    if (regexp) {
        painter.drawPixmap(x, 0, regexpIcon);
        x += regexpIcon.width() / dpr;
    }
    if (preservecase)
        painter.drawPixmap(x, 0, preservecaseIcon);
    return pixmap;
}

QString IFindFilter::descriptionForFindFlags(FindFlags flags)
{
    QStringList flagStrings;
    if (flags & FindCaseSensitively)
        flagStrings.append(tr("Case sensitive"));
    if (flags & FindWholeWords)
        flagStrings.append(tr("Whole words"));
    if (flags & FindRegularExpression)
        flagStrings.append(tr("Regular expressions"));
    if (flags & FindPreserveCase)
        flagStrings.append(tr("Preserve case"));
    QString description = tr("Flags: %1");
    if (flagStrings.isEmpty())
        description = description.arg(tr("None"));
    else
        description = description.arg(flagStrings.join(tr(", ")));
    return description;
}

} // namespace Core
