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

#include "icontext.h"

#include <QDebug>

QDebug operator<<(QDebug debug, const Core::Context &context)
{
    debug.nospace() << "Context(";
    Core::Context::const_iterator it = context.begin();
    Core::Context::const_iterator end = context.end();
    if (it != end) {
        debug << *it;
        ++it;
    }
    while (it != end) {
        debug << ", " << *it;
        ++it;
    }
    debug << ')';

    return debug;
}

/*!
    \class Core::Context
    \inmodule QtCreator
    \ingroup mainclasses
    \brief The Context class implements a list of context IDs.

    Contexts are used for registering actions with Core::ActionManager, and
    when creating UI elements that provide a context for actions.

    See \l{The Action Manager and Commands} for an overview of how contexts are
    used.

    \sa Core::IContext
    \sa Core::ActionManager
    \sa {The Action Manager and Commands}
*/

/*!
    \typedef Core::Context::const_iterator

    \brief The Context::const_iterator provides an STL-style const interator for
    Context.
*/

/*!
    \fn Core::Context::Context()

    Creates a context list that represents the global context.
*/

/*!
    \fn Core::Context::Context(Core::Id c1)

    Creates a context list with a single ID \a c1.
*/

/*!
    \fn Core::Context::Context(Core::Id c1, Core::Id c2)

    Creates a context list with IDs \a c1 and \a c2.
*/

/*!
    \fn Core::Context::Context(Core::Id c1, Core::Id c2, Core::Id c3)

    Creates a context list with IDs \a c1, \a c2 and \a c3.
*/

/*!
    \fn bool Core::Context::contains(Core::Id c) const

    Returns whether this context list contains the ID \a c.
*/

/*!
    \fn int Core::Context::size() const

    Returns the number of IDs in the context list.
*/

/*!
    \fn bool Core::Context::isEmpty() const

    Returns whether this context list is empty and therefore default
    constructed.
*/

/*!
    \fn Core::Id Core::Context::at(int i) const

    Returns the ID at index \a i in the context list.
*/

/*!
    \fn Core::Context::const_iterator Core::Context::begin() const

    Returns an STL-style iterator pointing to the first ID in the context list.
*/

/*!
    \fn Core::Context::const_iterator Core::Context::end() const

    Returns an STL-style iterator pointing to the imaginary item after the last
    ID in the context list.
*/

/*!
    \fn int Core::Context::indexOf(Core::Id c) const

    Returns the index position of the ID \a c in the context list. Returns -1
    if no item matched.
*/

/*!
    \fn void Core::Context::removeAt(int i)

    Removes the ID at index \a i from the context list.
*/

/*!
    \fn void Core::Context::prepend(Core::Id c)

    Adds the ID \a c as the first item to the context list.
*/

/*!
    \fn void Core::Context::add(const Core::Context &c)

    Adds the context list \a c at the end of this context list.
*/

/*!
    \fn void Core::Context::add(Core::Id c)

    Adds the ID \a c at the end of the context list.
*/

/*!
    \fn bool Core::Context::operator==(const Core::Context &c) const
    \internal
*/

/*!
    \class Core::IContext
    \inmodule QtCreator
    \ingroup mainclasses
    \brief The IContext class holds the context for performing an action.

    The behavior of some actions depends on the context in which they are
    applied.
*/
