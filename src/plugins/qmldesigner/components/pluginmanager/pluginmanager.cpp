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

#include "pluginmanager.h"
#include "iplugin.h"
#include "pluginpath.h"
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
        d(new PluginManagerPrivate)
{
}

PluginManager::~PluginManager()
{
    delete d;
}

PluginManager::IPluginList PluginManager::instances()
{
    IPluginList rc;
    const PluginManagerPrivate::PluginPathList::iterator end = d->m_paths.end();
    for (PluginManagerPrivate::PluginPathList::iterator it = d->m_paths.begin(); it != end; ++it)
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
        d->m_paths.push_back(PluginPath(dir));
    }

    // also register path in widgetpluginmanager
    MetaInfo::setPluginPaths(paths);
}

QAbstractItemModel *PluginManager::createModel(QObject *parent)
{
    QStandardItemModel *model = new QStandardItemModel(parent);
    const PluginManagerPrivate::PluginPathList::iterator end = d->m_paths.end();
    for (PluginManagerPrivate::PluginPathList::iterator it = d->m_paths.begin(); it != end; ++it)
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
