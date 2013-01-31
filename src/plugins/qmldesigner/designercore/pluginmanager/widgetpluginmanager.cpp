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

#include "widgetpluginmanager.h"
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

WidgetPluginManager::WidgetPluginManager()
{
}

WidgetPluginManager::IWidgetPluginList WidgetPluginManager::instances()
{
    IWidgetPluginList rc;
    const PluginPathList::iterator end = m_paths.end();
    for (PluginPathList::iterator it = m_paths.begin(); it != end; ++it)
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
    m_paths.push_back(WidgetPluginPath(dir));
    return true;
}

QAbstractItemModel *WidgetPluginManager::createModel(QObject *parent)
{
    QStandardItemModel *model = new QStandardItemModel(parent);
    const PluginPathList::iterator end = m_paths.end();
    for (PluginPathList::iterator it = m_paths.begin(); it != end; ++it)
        model->appendRow(it->createModelItem());
    return model;
}

} // namespace Internal
} // namespace QmlDesigner
