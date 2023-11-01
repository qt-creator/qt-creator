// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Falko Arps
// Copyright (C) 2016 Sven Klein
// Copyright (C) 2016 Giuliano Schneider
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "inavigationwidgetfactory.h"

#include <QIcon>
#include <QKeySequence>

/*!
    \class Core::INavigationWidgetFactory
    \inheaderfile coreplugin/inavigationwidgetfactory.h
    \ingroup mainclasses
    \inmodule QtCreator

    \brief The INavigationWidgetFactory class provides new instances of navigation widgets.

    A navigation widget factory is necessary because there can be more than one navigation widget of
    the same type at a time. Each navigation widget is wrapped in a \l{Core::NavigationView} for
    delivery.
*/

/*!
    \class Core::NavigationView
    \inheaderfile coreplugin/inavigationwidgetfactory.h
    \inmodule Qt Creator
    \brief The NavigationView class is a C struct for wrapping a widget and a list of tool buttons.
    Wrapping the widget that is shown in the content area of the navigation widget and a list of
    tool buttons that is shown in the header above it.
*/

/*!
    \fn QString Core::INavigationWidgetFactory::displayName() const

    Returns the display name of the navigation widget, which is shown in the dropdown menu above the
    navigation widget.
*/

/*!
    \fn int Core::INavigationWidgetFactory::priority() const

    Determines the position of the navigation widget in the dropdown menu.

    0 to 1000 from top to bottom
*/

/*!
    \fn Id Core::INavigationWidgetFactory::id() const

    Returns a unique identifier for referencing the navigation widget factory.
*/

/*!
    \fn Core::NavigationView Core::INavigationWidgetFactory::createWidget()

    Returns a \l{Core::NavigationView} containing the widget and the buttons. The ownership is given
    to the caller.
*/

using namespace Utils;

namespace Core {

static QList<INavigationWidgetFactory *> g_navigationWidgetFactories;

/*!
    Constructs a navigation widget factory.
*/
INavigationWidgetFactory::INavigationWidgetFactory()
{
    g_navigationWidgetFactories.append(this);
}

INavigationWidgetFactory::~INavigationWidgetFactory()
{
    g_navigationWidgetFactories.removeOne(this);
}

const QList<INavigationWidgetFactory *> INavigationWidgetFactory::allNavigationFactories()
{
    return g_navigationWidgetFactories;
}

/*!
    Sets the display name for the factory to \a displayName.

    \sa displayName()
*/
void INavigationWidgetFactory::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
}

/*!
    Sets the \a priority for the factory.

    \sa priority()
*/
void INavigationWidgetFactory::setPriority(int priority)
{
    m_priority = priority;
}

/*!
    Sets the \a id for the factory.

    \sa id()
*/
void INavigationWidgetFactory::setId(Id id)
{
    m_id = id;
}

/*!
    Sets the keyboard activation sequence for the factory to \a keys.

    \sa activationSequence()
*/
void INavigationWidgetFactory::setActivationSequence(const QKeySequence &keys)
{
    m_activationSequence = keys;
}

/*!
    Returns the keyboard shortcut to activate an instance of a navigation widget.
*/
QKeySequence INavigationWidgetFactory::activationSequence() const
{
    return m_activationSequence;
}

/*!
    Stores the \a settings for the \a widget at \a position that was created by this factory
    (the \a position identifies a specific navigation widget).

    \sa INavigationWidgetFactory::restoreSettings()
*/
void INavigationWidgetFactory::saveSettings(QtcSettings * /* settings */,
                                            int /* position */,
                                            QWidget * /* widget */)
{
}

/*!
    Reads and restores the \a settings for the \a widget at \a position that was created by this
    factory (the \a position identifies a specific navigation widget).

    \sa INavigationWidgetFactory::saveSettings()
*/
void INavigationWidgetFactory::restoreSettings(QtcSettings * /* settings */, int /* position */,
                                               QWidget * /* widget */)
{
}

// Registers a new root path in the factory
void INavigationWidgetFactory::addRootPath(Id /*id*/, const QString & /*displayName*/,
                                           const QIcon & /*icon*/, const FilePath & /*path*/)
{
}

// Removes a root path from the factory
void INavigationWidgetFactory::removeRootPath(Id /*path*/)
{
}

} // Core
