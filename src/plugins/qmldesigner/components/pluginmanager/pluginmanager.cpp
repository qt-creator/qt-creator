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

#include "pluginmanager.h"
#include "iplugin.h"
#include <metainfo.h>

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
#include <QTreeView>
#include <QVBoxLayout>
#include <QDialog>
#include <QLabel>
#include <QDialogButtonBox>

enum { debug = 0 };

namespace QmlDesigner {

PluginManager::IPluginList PluginManager::instances()
{
    IPluginList rc;
    const PluginPathList::iterator end = m_paths.end();
    for (PluginPathList::iterator it = m_paths.begin(); it != end; ++it)
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
    const PluginPathList::iterator end = m_paths.end();
    for (PluginPathList::iterator it = m_paths.begin(); it != end; ++it)
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
