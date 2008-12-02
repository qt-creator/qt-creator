/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#include "basemode.h"

#include <QtGui/QWidget>

#include <extensionsystem/pluginmanager.h>

using namespace Core;

/*!
    \class BaseMode
    \mainclass
    \ingroup qwb
    \inheaderfile basemode.h
    \brief A base implementation of the mode interface IMode.

    The BaseMode class can be used directly for most IMode implementations. It has setter functions
    for the mode properties and a convenience constructor.

    The ownership of the widget is given to the BaseMode, so when the BaseMode is destroyed it
    deletes its widget.

    A typical use case is to do the following in the init method of a plugin:
    \code
    bool MyPlugin::init(QString *error_message)
    {
        [...]
        addObject(new Core::BaseMode("mymode",
            "MyPlugin.UniqueModeName",
            icon,
            50, // priority
            new MyWidget));
        [...]
    }
    \endcode
*/

/*!
    \fn BaseMode::BaseMode(QObject *parent)

    Creates a mode with empty name, no icon, lowest priority and no widget. You should use the
    setter functions to give the mode a meaning.

    \a parent
*/
BaseMode::BaseMode(QObject *parent):
    IMode(parent),
    m_priority(0),
    m_widget(0)
{
}

/*!
    \fn BaseMode::BaseMode(const QString &name,
                           const char * uniqueModeName,
                           const QIcon &icon,
                           int priority,
                           QWidget *widget,
                           QObject *parent)

    Creates a mode with the given properties.

    \a name
    \a uniqueModeName
    \a icon
    \a priority
    \a widget
    \a parent
*/
BaseMode::BaseMode(const QString &name,
                   const char * uniqueModeName,
                   const QIcon &icon,
                   int priority,
                   QWidget *widget,
                   QObject *parent):
    IMode(parent),
    m_name(name),
    m_icon(icon),
    m_priority(priority),
    m_widget(widget),
    m_uniqueModeName(uniqueModeName)
{
}

/*!
    \fn BaseMode::~BaseMode()
*/
BaseMode::~BaseMode()
{
    delete m_widget;
}
