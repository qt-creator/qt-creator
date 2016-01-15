/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "pluginmanager.h"
#include "iplugin.h"
#include <metainfo.h>

#include <QCoreApplication>
#include <QObject>
#include <QDir>
#include <QStringList>
#include <QDebug>

#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>

enum { debug = 0 };

namespace QmlDesigner {

PluginManager::IPluginList PluginManager::instances()
{
    IPluginList rc;
    const auto end = m_paths.end();
    for (auto it = m_paths.begin(); it != end; ++it)
        it->getInstances(&rc);
    if (debug)
        qDebug() << '<' << Q_FUNC_INFO << rc.size();
    return rc;
}

PluginManager::PluginManager()
{
}

void PluginManager::setPluginPaths(const QStringList &paths)
{
    foreach (const QString &path, paths) {
        const QDir dir(path);
        if (!dir.exists())
            continue;
        m_paths.push_back(PluginPath(dir));
    }

    // also register path in widgetpluginmanager
    MetaInfo::setPluginPaths(paths);
}

QAbstractItemModel *PluginManager::createModel(QObject *parent)
{
    QStandardItemModel *model = new QStandardItemModel(parent);
    const auto end = m_paths.end();
    for (auto it = m_paths.begin(); it != end; ++it)
        model->appendRow(it->createModelItem());
    return model;
}

QDialog *PluginManager::createAboutPluginDialog(QWidget *parent)
{
    QDialog *rc = new QDialog(parent);
    rc->setWindowFlags(rc->windowFlags() & ~Qt::WindowContextHelpButtonHint & Qt::Sheet);
    rc->setWindowTitle(QCoreApplication::translate("QmlDesigner::PluginManager", "About Plugins"));
    QTreeView *treeView = new QTreeView;
    treeView->setModel(createModel(treeView));
    treeView->expandAll();

    QVBoxLayout *layout = new QVBoxLayout(rc);
    layout->addWidget(treeView);
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Close);
    layout->addWidget(bb);
    QObject::connect(bb, SIGNAL(rejected()), rc, SLOT(reject()));
    return rc;
}

} // namespace QmlDesigner
