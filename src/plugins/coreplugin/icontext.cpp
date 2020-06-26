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
    \inheaderfile coreplugin/icontext.h
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
    \fn Core::Context::Context(Utils::Id c1)

    Creates a context list with a single ID \a c1.
*/

/*!
    \fn Core::Context::Context(Utils::Id c1, Utils::Id c2)

    Creates a context list with IDs \a c1 and \a c2.
*/

/*!
    \fn Core::Context::Context(Utils::Id c1, Utils::Id c2, Utils::Id c3)

    Creates a context list with IDs \a c1, \a c2 and \a c3.
*/

/*!
    \fn bool Core::Context::contains(Utils::Id c) const

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
    \fn Utils::Id Core::Context::at(int i) const

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
    \fn int Core::Context::indexOf(Utils::Id c) const

    Returns the index position of the ID \a c in the context list. Returns -1
    if no item matched.
*/

/*!
    \fn void Core::Context::removeAt(int i)

    Removes the ID at index \a i from the context list.
*/

/*!
    \fn void Core::Context::prepend(Utils::Id c)

    Adds the ID \a c as the first item to the context list.
*/

/*!
    \fn void Core::Context::add(const Core::Context &c)

    Adds the context list \a c at the end of this context list.
*/

/*!
    \fn void Core::Context::add(Utils::Id c)

    Adds the ID \a c at the end of the context list.
*/

/*!
    \fn bool Core::Context::operator==(const Core::Context &c) const
    \internal
*/

/*!
    \class Core::IContext
    \inheaderfile coreplugin/icontext.h
    \inmodule QtCreator
    \ingroup mainclasses

    \brief The IContext class associates a widget with a context list and
    context help.

    An instance of IContext must be registered with
    Core::ICore::addContextObject() to have an effect. For many subclasses of
    IContext, like Core::IEditor and Core::IMode, this is done automatically.
    But instances of IContext can be created manually to associate a context
    and context help for an arbitrary widget, too. IContext instances are
    automatically unregistered when they are deleted. Use
    Core::ICore::removeContextObject() if you need to unregister an IContext
    instance manually.

    Whenever the widget is part of the application wide focus widget's parent
    chain, the associated context list is made active. This makes actions active
    that were registered for any of the included context IDs. If the user
    requests context help, the top-most IContext instance in the focus widget's
    parent hierarchy is asked to provide it.

    See \l{The Action Manager and Commands} for an overview of how contexts are
    used for managing actions.

    \sa Core::ICore
    \sa Core::Context
    \sa Core::ActionManager
    \sa {The Action Manager and Commands}
*/

/*!
    \fn Core::IContext::IContext(QObject *parent)

    Creates an IContext with an optional \a parent.
*/

/*!
    \fn Core::Context Core::IContext::context() const

    Returns the context list associated with this IContext.

    \sa setContext()
*/

/*!
    \fn QWidget *Core::IContext::widget() const

    Returns the widget associated with this IContext.

    \sa setWidget()
*/

/*!
    \typedef Core::IContext::HelpCallback

    The HelpCallback class defines the callback function that is used to report
    the help item to show when the user requests context help.
*/

/*!
    \fn void Core::IContext::contextHelp(const Core::IContext::HelpCallback &callback) const

    Called when the user requests context help and this IContext is the top-most
    in the application focus widget's parent hierarchy. Implementations must
    call the passed \a callback with the resulting help item.
    The default implementation returns an help item with the help ID that was
    set with setContextHelp().

    \sa setContextHelp()
*/

/*!
    \fn void Core::IContext::setContext(const Core::Context &context)

    Sets the context list associated with this IContext to \a context.

    \sa context()
*/

/*!
    \fn void Core::IContext::setWidget(QWidget *widget)

    Sets the widget associated with this IContext to \a widget.

    \sa widget()
*/

/*!
    \fn void Core::IContext::setContextHelp(const Core::HelpItem &id)

    Sets the context help item associated with this IContext to \a id.

    \sa contextHelp()
*/
