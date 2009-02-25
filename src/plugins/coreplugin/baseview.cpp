/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "baseview.h"

#include <QtGui/QWidget>

using namespace Core;

/*!
    \class BaseView
    \mainclass
    \ingroup qwb
    \inheaderfile baseview.h
    \brief A base implementation of IView.

    The BaseView class can be used directly for most IView implementations.
    It has setter functions for the views properties, and a convenience constructor
    for the most important ones.

    The ownership of the widget is given to the BaseView, so when the BaseView is destroyed it
    deletes its widget.

    A typical use case is to do the following in the init method of a plugin:
    \code
    bool MyPlugin::init(QString *error_message)
    {
        [...]
        addObject(new Core::BaseView("myplugin.myview",
            new MyWidget,
            QList<int>() << myContextId,
            Qt::LeftDockWidgetArea,
            this));
        [...]
    }
    \endcode
*/

/*!
    \fn BaseView::BaseView()

    Creates a View with empty view name, no widget, empty context, NoDockWidgetArea,
    no menu group and empty default shortcut. You should use the setter functions
    to give the view a meaning.
*/
BaseView::BaseView(QObject *parent)
        : IView(parent),
    m_viewName(""),
    m_widget(0),
    m_context(QList<int>()),
    m_defaultPosition(IView::First)
{
}

/*!
    \fn BaseView::~BaseView()

    Destructor also destroys the widget.
*/
BaseView::~BaseView()
{
    delete m_widget;
}

/*!
    \fn const QList<int> &BaseView::context() const
*/
QList<int> BaseView::context() const
{
    return m_context;
}

/*!
    \fn QWidget *BaseView::widget()
*/
QWidget *BaseView::widget()
{
    return m_widget;
}

/*!
    \fn const char *BaseView::uniqueViewName() const
*/
const char *BaseView::uniqueViewName() const
{
    return m_viewName;
}


/*!
    \fn IView::ViewPosition BaseView::defaultPosition() const
*/
IView::ViewPosition BaseView::defaultPosition() const
{
    return m_defaultPosition;
}

/*!
    \fn void BaseView::setUniqueViewName(const char *name)

    \a name
*/
void BaseView::setUniqueViewName(const char *name)
{
    m_viewName = name;
}

/*!
    \fn QWidget *BaseView::setWidget(QWidget *widget)

    The BaseView takes the ownership of the \a widget. The previously
    set widget (if existent) is not deleted but returned, and responsibility
    for deleting it moves to the caller of this method.
*/
QWidget *BaseView::setWidget(QWidget *widget)
{
    QWidget *oldWidget = m_widget;
    m_widget = widget;
    return oldWidget;
}

/*!
    \fn void BaseView::setContext(const QList<int> &context)

    \a context
*/
void BaseView::setContext(const QList<int> &context)
{
    m_context = context;
}

/*!
    \fn void BaseView::setDefaultPosition(IView::ViewPosition position)

    \a position
*/
void BaseView::setDefaultPosition(IView::ViewPosition position)
{
    m_defaultPosition = position;
}

