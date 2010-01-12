/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "widgetpluginmanager.h"
#include "widgetpluginpath.h"
#include <iwidgetplugin.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QSharedData>
#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtCore/QDebug>
#include <QWeakPointer>
#include <QtCore/QPluginLoader>
#include <QtCore/QFileInfo>
#include <QtCore/QLibraryInfo>

#include <QtGui/QStandardItemModel>
#include <QtGui/QStandardItem>

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
        m_d(new WidgetPluginManagerPrivate)
{
}

WidgetPluginManager::~WidgetPluginManager()
{
    delete m_d;
}

WidgetPluginManager::IWidgetPluginList WidgetPluginManager::instances()
{
    if (debug)
        qDebug() << '>' << Q_FUNC_INFO << QLibraryInfo::buildKey();
    IWidgetPluginList rc;
    const WidgetPluginManagerPrivate::PluginPathList::iterator end = m_d->m_paths.end();
    for (WidgetPluginManagerPrivate::PluginPathList::iterator it = m_d->m_paths.begin(); it != end; ++it)
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
    m_d->m_paths.push_back(WidgetPluginPath(dir));
    return true;
}

QAbstractItemModel *WidgetPluginManager::createModel(QObject *parent)
{
    QStandardItemModel *model = new QStandardItemModel(parent);
    const WidgetPluginManagerPrivate::PluginPathList::iterator end = m_d->m_paths.end();
    for (WidgetPluginManagerPrivate::PluginPathList::iterator it = m_d->m_paths.begin(); it != end; ++it)
        model->appendRow(it->createModelItem());
    return model;
}

} // namespace Internal
} // namespace QmlDesigner
