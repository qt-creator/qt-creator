/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "widgetpluginmanager.h"

#include <QObject>
#include <QDir>
#include <QDebug>

#include <QStandardItemModel>

enum { debug = 0 };

namespace QmlDesigner {

namespace Internal {

WidgetPluginManager::WidgetPluginManager()
{
}

WidgetPluginManager::IWidgetPluginList WidgetPluginManager::instances()
{
    IWidgetPluginList rc;
    const auto end = m_paths.end();
    for (auto it = m_paths.begin(); it != end; ++it)
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
    const auto end = m_paths.end();
    for (auto it = m_paths.begin(); it != end; ++it)
        model->appendRow(it->createModelItem());
    return model;
}

} // namespace Internal
} // namespace QmlDesigner
