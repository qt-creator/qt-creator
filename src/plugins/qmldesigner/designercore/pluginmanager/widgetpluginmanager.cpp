/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "widgetpluginmanager.h"
#include "widgetpluginpath.h"
#include <iwidgetplugin.h>

#include <QCoreApplication>
#include <QObject>
#include <QSharedData>
#include <QDir>
#include <QStringList>
#include <QDebug>
#include <QWeakPointer>
#include <QPluginLoader>
#include <QFileInfo>
#include <QLibraryInfo>

#include <QStandardItemModel>
#include <QStandardItem>

enum { debug = 0 };

namespace QmlDesigner {

namespace Internal {

// ---- PluginManager[Private]
class WidgetPluginManagerPrivate {
public:
    typedef QList<WidgetPluginPath> PluginPathList;
    PluginPathList m_paths;
};

WidgetPluginManager::WidgetPluginManager() :
        d(new WidgetPluginManagerPrivate)
{
}

WidgetPluginManager::~WidgetPluginManager()
{
    delete d;
}

WidgetPluginManager::IWidgetPluginList WidgetPluginManager::instances()
{
    IWidgetPluginList rc;
    const WidgetPluginManagerPrivate::PluginPathList::iterator end = d->m_paths.end();
    for (WidgetPluginManagerPrivate::PluginPathList::iterator it = d->m_paths.begin(); it != end; ++it)
        it->getInstances(&rc);
    if (debug)
        qDebug() << '<' << Q_FUNC_INFO << rc.size();
    return rc;
}

bool WidgetPluginManager::addPath(const QString &path)
{
    const QDir dir(path);
    if (!dir.exists())
        return false;
    d->m_paths.push_back(WidgetPluginPath(dir));
    return true;
}

QAbstractItemModel *WidgetPluginManager::createModel(QObject *parent)
{
    QStandardItemModel *model = new QStandardItemModel(parent);
    const WidgetPluginManagerPrivate::PluginPathList::iterator end = d->m_paths.end();
    for (WidgetPluginManagerPrivate::PluginPathList::iterator it = d->m_paths.begin(); it != end; ++it)
        model->appendRow(it->createModelItem());
    return model;
}

} // namespace Internal
} // namespace QmlDesigner
