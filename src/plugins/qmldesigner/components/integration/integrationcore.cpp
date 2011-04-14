/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "integrationcore.h"
#include "pluginmanager.h"
#include "itemlibraryview.h"
#include "navigatorwidget.h"
#include "metainfo.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QObject>

namespace QmlDesigner {

class CorePrivate
{
    public:
    CorePrivate();
    ~CorePrivate();

    static IntegrationCore *m_instance;

    PluginManager m_pluginManager;
};

CorePrivate::CorePrivate()
{
}

CorePrivate::~CorePrivate()
{
    MetaInfo::clearGlobal();
}

IntegrationCore *CorePrivate::m_instance = 0;

/*!
  \class QmlDesigner::Core

   Convenience class to access the plugin manager singleton,
   and manage the lifecycle of static data (e.g. in the metatype system).
*/

IntegrationCore::IntegrationCore() :
   m_d(new CorePrivate)
{
    Q_ASSERT(CorePrivate::m_instance == 0);
    CorePrivate::m_instance = this;
}

IntegrationCore::~IntegrationCore()
{
    CorePrivate::m_instance = 0;
    delete m_d;
}

IntegrationCore *IntegrationCore::instance()
{
    Q_ASSERT(CorePrivate::m_instance);
    return CorePrivate::m_instance;
}

PluginManager *IntegrationCore::pluginManager() const
{
    return &m_d->m_pluginManager;
}

}
