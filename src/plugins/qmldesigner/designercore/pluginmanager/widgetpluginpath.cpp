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

#include "widgetpluginpath.h"
#include <iwidgetplugin.h>
#include <QLibrary>
#include <QWeakPointer>
#include <QPluginLoader>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QCoreApplication>
#include <QObject>
#include <QSharedData>
#include <QDebug>

enum { debug = 0 };

namespace QmlDesigner {

namespace Internal {

// Initialize and create instance of a plugin from scratch,
// that is, make sure the library is loaded and has an instance
// of the IPlugin type. Once something fails, mark it as failed
// ignore it from then on.
static IWidgetPlugin *instance(WidgetPluginData &p)
{
    if (debug)
        qDebug() << "Loading QmlDesigner plugin" << p.path << "...";

    // Go stale once something fails
    if (p.failed)
        return 0;

    // Pull up the plugin, retrieve IPlugin instance.
    if (!p.instanceGuard) {
        p.instance = 0;
        QPluginLoader loader(p.path);

        if (debug)
            qDebug() << "guard" << p.path;

        if (!(loader.isLoaded() || loader.load())) {
            p.failed = true;
            p.errorMessage = QCoreApplication::translate("WidgetPluginManager",
                                                         "Failed to create instance of file "
                                                         "'%1': %2").arg(p.path).arg(p.errorMessage);
            qWarning() << p.errorMessage;
            return 0;
        }
        QObject *object = loader.instance();
        if (!object) {
            p.failed = true;
            p.errorMessage = QCoreApplication::translate("WidgetPluginManager",
                                                         "Failed to create instance of file '%1'."
                                                         ).arg(p.path);
            qWarning() << p.errorMessage;
            return 0;
        }
        IWidgetPlugin *iplugin = qobject_cast<IWidgetPlugin *>(object);
        if (!iplugin) {
            p.failed = true;
            p.errorMessage = QCoreApplication::translate("WidgetPluginManager",
                                                         "File '%1' is not a QmlDesigner plugin."
                                                         ).arg(p.path);
            qWarning() << p.errorMessage;
            delete object;
            return 0;
        }
        p.instanceGuard = object;
        p.instance = iplugin;
    }
    // Ensure it is initialized
    /*if (!p.instance->isInitialized()) {
        if (!p.instance->initialize(&p.errorMessage)) {
            p.failed = true;
            delete p.instance;
            p.instance = 0;
            return 0;
        }
    }*/

    if (debug)
        qDebug() << "QmlDesigner plugin" << p.path << "successfully loaded!";
    return p.instance;
}

WidgetPluginData::WidgetPluginData(const QString &p) :
    path(p),
    failed(false),
    instance(0)
{
}


WidgetPluginPath::WidgetPluginPath(const QDir &path) :
    m_path(path),
    m_loaded(false)
{
}

// Determine a unique list of library files in that directory
QStringList WidgetPluginPath::libraryFilePaths(const QDir &dir)
{
    const QFileInfoList infoList = dir.entryInfoList(QDir::Files|QDir::Readable|QDir::NoDotAndDotDot);
    if (infoList.empty())
        return QStringList();
      // Load symbolic links but make sure all file names are unique as not
    // to fall for something like 'libplugin.so.1 -> libplugin.so'
    QStringList result;
    const QFileInfoList::const_iterator icend = infoList.constEnd();
    for (QFileInfoList::const_iterator it = infoList.constBegin(); it != icend; ++it) {
        QString fileName;
        if (it->isSymLink()) {
            const QFileInfo linkTarget = QFileInfo(it->symLinkTarget());
            if (linkTarget.exists() && linkTarget.isFile())
                fileName = linkTarget.absoluteFilePath();
        } else {
            fileName = it->absoluteFilePath();
        }
        if (!fileName.isEmpty() && QLibrary::isLibrary(fileName) && !result.contains(fileName))
            result += fileName;
    }

    if (debug)
        qDebug() << "Library files in directory" << dir << ": " << result;

    return result;
}

void WidgetPluginPath::clear()
{
    m_loaded = false;
    m_plugins.clear();
}

void WidgetPluginPath::ensureLoaded()
{
    if (!m_loaded) {
        const QStringList libraryFiles = libraryFilePaths(m_path);
        if (debug)
            qDebug() << "Checking " << libraryFiles.size() << " plugins " << m_path.absolutePath();
        foreach (const QString &libFile, libraryFiles)
            m_plugins.push_back(WidgetPluginData(libFile));
        m_loaded = true;
    }
}

void WidgetPluginPath::getInstances(IWidgetPluginList *list)
{
    ensureLoaded();
    // Compile list of instances
    if (m_plugins.empty())
        return;
    const PluginDataList::iterator end = m_plugins.end();
    for (PluginDataList::iterator it = m_plugins.begin(); it != end; ++it)
        if (IWidgetPlugin *i = instance(*it))
            list->push_back(i);
}

QStandardItem *WidgetPluginPath::createModelItem()
{
    ensureLoaded();
    // Create a list of plugin lib files with classes.
    // If there are failed ones, create a separate "Failed"
    // category at the end
    QStandardItem *pathItem = new QStandardItem(m_path.absolutePath());
    QStandardItem *failedCategory = 0;
    const PluginDataList::iterator end = m_plugins.end();
    for (PluginDataList::iterator it = m_plugins.begin(); it != end; ++it) {
        QStandardItem *pluginItem = new QStandardItem(QFileInfo(it->path).fileName());
        if (instance(*it)) {
            pluginItem->appendRow(new QStandardItem(QString::fromLatin1(it->instanceGuard->metaObject()->className())));
            pathItem->appendRow(pluginItem);
        } else {
            pluginItem->setToolTip(it->errorMessage);
            if (!failedCategory) {
                const QString failed = QCoreApplication::translate("PluginManager", "Failed Plugins");
                failedCategory = new QStandardItem(failed);
            }
            failedCategory->appendRow(pluginItem);
        }
    }
    if (failedCategory)
        pathItem->appendRow(failedCategory);
    return pathItem;
}

} // namespace Internal
} // namespace QmlDesigner

