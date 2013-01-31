/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
    foreach (const QString &name, styles())
        if ((style = QStyleFactory::create(name)) &&
            (applicationStyle->metaObject()->className() ==
                        style->metaObject()->className()))
          return name;
    return QString();
}





} //namespace QmlDesigner
