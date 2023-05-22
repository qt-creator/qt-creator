// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ifindsupport.h"

#include <utils/fadingindicator.h>
#include <utils/stylehelper.h>

using namespace Core;
using namespace Utils;

/*!
    \class Core::IFindSupport
    \inheaderfile coreplugin/find/ifindsupport.h
    \inmodule QtCreator

    \brief The IFindSupport class provides functions for searching in a document
     or widget.

    \sa Core::BaseTextFind
*/

/*!
    \enum Core::IFindSupport::Result
    This enum holds whether the search term was found within the search scope
    using the find flags.

    \value Found        The search term was found.
    \value NotFound     The search term was not found.
    \value NotYetFound  The search term has not been found yet.
*/

/*!
    \fn Core::IFindSupport::IFindSupport()
    \internal
*/

/*!
    \fn Core::IFindSupport::~IFindSupport()
    \internal
*/

/*!
    \fn bool Core::IFindSupport::supportsReplace() const
    Returns whether the find filter supports search and replace.
*/

/*!
    \fn bool Core::IFindSupport::supportsSelectAll() const
    Returns whether the find filter supports selecting all results.
*/
bool IFindSupport::supportsSelectAll() const
{
    return false;
}

/*!
    \fn Utils::FindFlags Core::IFindSupport::supportedFindFlags() const
    Returns the find flags, such as whole words or regular expressions,
    that this find filter supports.

    Depending on the returned value, the default find option widgets are
    enabled or disabled.

    The default is Uitls::FindBackward, Utils::FindCaseSensitively,
    Uitls::FindRegularExpression, Uitls::FindWholeWords, and
    Uitls::FindPreserveCase.
*/

/*!
    \fn void Core::IFindSupport::resetIncrementalSearch()
    Resets incremental search to start position.
*/

/*!
    \fn void Core::IFindSupport::clearHighlights()
    Clears highlighting of search results in the searched widget.
*/

/*!
    \fn QString Core::IFindSupport::currentFindString() const
    Returns the current search string.
*/

/*!
    \fn QString Core::IFindSupport::completedFindString() const
    Returns the complete search string.
*/

/*!
    \fn void Core::IFindSupport::highlightAll(const QString &txt, Utils::FindFlags findFlags)
    Highlights all search hits for \a txt when using \a findFlags.
*/

/*!
    \fn Core::IFindSupport::Result Core::IFindSupport::findIncremental(const QString &txt, Utils::FindFlags findFlags)
    Performs an incremental search of the search term \a txt using \a findFlags.
*/

/*!
    \fn Core::IFindSupport::Result Core::IFindSupport::findStep(const QString &txt, Utils::FindFlags findFlags)
    Searches for \a txt using \a findFlags.
*/

/*!
    \fn void Core::IFindSupport::defineFindScope()
    Defines the find scope.
*/

/*!
    \fn void Core::IFindSupport::clearFindScope()
    Clears the find scope.
*/

/*!
    \fn void Core::IFindSupport::changed()
    This signal is emitted when the search changes.
*/

/*!
    Replaces \a before with \a after as specified by \a findFlags.
*/
void IFindSupport::replace(const QString &before, const QString &after, FindFlags findFlags)
{
    Q_UNUSED(before)
    Q_UNUSED(after)
    Q_UNUSED(findFlags)
}

/*!
    Replaces \a before with \a after as specified by \a findFlags, and then
    performs findStep().

    Returns whether the find step found another match.
*/
bool IFindSupport::replaceStep(const QString &before, const QString &after, FindFlags findFlags)
{
    Q_UNUSED(before)
    Q_UNUSED(after)
    Q_UNUSED(findFlags)
    return false;
}

/*!
    Finds and replaces all instances of \a before with \a after as specified by
    \a findFlags.
*/
int IFindSupport::replaceAll(const QString &before, const QString &after, FindFlags findFlags)
{
    Q_UNUSED(before)
    Q_UNUSED(after)
    Q_UNUSED(findFlags)
    return 0;
}

/*!
    Finds and selects all instances of \a txt with specified \a findFlags.
*/
void IFindSupport::selectAll(const QString &txt, FindFlags findFlags)
{
    Q_UNUSED(txt)
    Q_UNUSED(findFlags)
}

/*!
    Shows \a parent overlayed with the wrap indicator.
*/
void IFindSupport::showWrapIndicator(QWidget *parent)
{
    FadingIndicator::showPixmap(parent, StyleHelper::dpiSpecificImageFile(
                                            ":/find/images/wrapindicator.png"));
}
