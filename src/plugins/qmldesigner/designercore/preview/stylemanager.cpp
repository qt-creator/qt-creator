/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "stylemanager.h"
#include <QStyleFactory>
#include <QApplication>
#include <QStyle>

namespace QmlDesigner {

namespace Internal {

// TODO KAI: REMOVE THIS CLASS

//### if we use this pattern often: make a template out of this!
class StyleManagerGuard { //This guard destroys the singleton in its destructor
public:                   //This should avoid that a memory leak is reported
   ~StyleManagerGuard() {
   if (StyleManager::m_instance != 0)
     delete StyleManager::m_instance;
   }
};
} //namespace Internal

StyleManager* StyleManager::m_instance = 0;

void StyleManager::addView(NodeInstanceView* view)
{
    instance()->m_views.append(view);
}

void StyleManager::removeView(NodeInstanceView* view)
{
    instance()->m_views.removeAll(view);
}

QStringList StyleManager::styles()
{
    return QStyleFactory::keys();
}

void StyleManager::setStyle(const QString &styleName)
{
    QStyle *style = QStyleFactory::create(styleName);
    if (style) {
        foreach (NodeInstanceView* view, instance()->m_views)
            view->setStyle(style);
    }
}

StyleManager* StyleManager::instance()
{
    static Internal::StyleManagerGuard guard; //The destructor destroys the singleton. See above
    if (m_instance == 0)
        m_instance = new StyleManager();
    return m_instance;
}

QString StyleManager::applicationStyle()
{
    QStyle *applicationStyle = qApp->style();
    QStyle *style;
    if (applicationStyle)
    foreach(const QString &name, styles())
        if ((style = QStyleFactory::create(name)) &&
            (applicationStyle->metaObject()->className() ==
                        style->metaObject()->className()))
          return name;
    return QString();
}





} //namespace QmlDesigner
