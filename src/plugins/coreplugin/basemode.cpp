/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "basemode.h"

#include <extensionsystem/pluginmanager.h>

#include <QtGui/QWidget>
#include <QtGui/QIcon>

using namespace Core;

/*!
    \class BaseMode
    \mainclass
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
            "MyPlugin.ModeId",
            icon,
            50, // priority
            new MyWidget));
        [...]
    }
    \endcode
*/

/*!
    \fn BaseMode::BaseMode(QObject *parent)

    Creates a mode with empty display name, no icon, lowest priority and no widget. You should use the
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
    \fn BaseMode::~BaseMode()
*/
BaseMode::~BaseMode()
{
    delete m_widget;
}
