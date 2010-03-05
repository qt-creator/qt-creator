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

#include "pluginmanager.h"
#include "iplugin.h"
#include "pluginpath.h"
#include <metainfo.h>

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
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include <QtGui/QDialog>
#include <QtGui/QLabel>
#include <QtGui/QDialogButtonBox>

enum { debug = 0 };

namespace QmlDesigner {

// Initialize and create instance of a plugin from scratch,
// that is, make sure the library is loaded and has an instance
// of the IPlugin type. Once something fails, mark it as failed
// ignore it from then on.
//static IPlugin *instance(PluginData &p)
//{
//    // Go stale once something fails
//    if (p.failed)
//        return 0;
//    // Pull up the plugin, retrieve IPlugin instance.
//    if (!p.instanceGuard) {
//        p.instance = 0;
//        QPluginLoader loader(p.path);
//        if (!(loader.isLoaded() || loader.load())) {
//            p.failed = true;
//            p.errorMessage = loader.errorString();
//            return 0;
//        }
//        QObject *object = loader.instance();
//        if (!object) {
//            p.failed = true;
//            p.errorMessage = QCoreApplication::translate("PluginManager", "Failed to create instance.");
//            return 0;
//        }
//        IPlugin *iplugin = qobject_cast<IPlugin *>(object);
//        if (!iplugin) {
//            p.failed = true;
//            p.errorMessage = QCoreApplication::translate("PluginManager", "Not a QmlDesigner plugin.");
//            delete object;
//            return 0;
//        }
//        p.instanceGuard = object;
//        p.instance = iplugin;
//    }
//    // Ensure it is initialized
//    if (!p.instance->isInitialized()) {
//        if (!p.instance->initialize(&p.errorMessage)) {
//            p.failed = true;
//            delete p.instance;
//            p.instance = 0;
//            return 0;
//        }
//    }
//    return p.instance;
//}


// ---- PluginManager[Private]
class PluginManagerPrivate {
public:
    typedef QList<PluginPath> PluginPathList;
    PluginPathList m_paths;
};

PluginManager::PluginManager() :
        m_d(new PluginManagerPrivate)
{
}

PluginManager::~PluginManager()
{
    delete m_d;
}

PluginManager::IPluginList PluginManager::instances()
{
    if (debug)
        qDebug() << '>' << Q_FUNC_INFO << QLibraryInfo::buildKey();
    IPluginList rc;
    const PluginManagerPrivate::PluginPathList::iterator end = m_d->m_paths.end();
    for (PluginManagerPrivate::PluginPathList::iterator it = m_d->m_paths.begin(); it != end; ++it)
        it->getInstances(&rc);
    if (debug)
        qDebug() << '<' << Q_FUNC_INFO << rc.size();
    return rc;
}

void PluginManager::setPluginPaths(const QStringList &paths)
{
    foreach (const QString &path, paths) {
        const QDir dir(path);
        if (!dir.exists())
            continue;
        m_d->m_paths.push_back(PluginPath(dir));
    }

    // also register path in widgetpluginmanager
    MetaInfo::setPluginPaths(paths);
}

QAbstractItemModel *PluginManager::createModel(QObject *parent)
{
    QStandardItemModel *model = new QStandardItemModel(parent);
    const PluginManagerPrivate::PluginPathList::iterator end = m_d->m_paths.end();
    for (PluginManagerPrivate::PluginPathList::iterator it = m_d->m_paths.begin(); it != end; ++it)
        model->appendRow(it->createModelItem());
    return model;
}

QDialog *PluginManager::createAboutPluginDialog(QWidget *parent)
{
    QDialog *rc = new QDialog(parent);
    rc->setWindowFlags(rc->windowFlags() & ~Qt::WindowContextHelpButtonHint & Qt::Sheet);
    rc->setWindowTitle(QCoreApplication::translate("QmlDesigner::PluginManager", "About plugins"));
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
