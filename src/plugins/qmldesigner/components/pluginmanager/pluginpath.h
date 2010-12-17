/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PLUGINPATH_H
#define PLUGINPATH_H

#include "pluginmanager.h"

#include <QtCore/QObject>
#include <QWeakPointer>
#include <QtCore/QList>
#include <QtCore/QDir>
#include <QtGui/QStandardItem>

QT_BEGIN_NAMESPACE
class QString;
class QAbstractItemModel;
QT_END_NAMESPACE

namespace QmlDesigner {

class IPlugin;

// Dumb plugin data structure. Note that whereas QObjects can
// casted to an interface, QPointer does not work with the
// interface class, so, we need a separate QPointer as a guard
// to detect the deletion of a plugin instance which can happen
// in theory.
struct PluginData {
    PluginData(const QString &p = QString());

    QString path;
    bool failed;
    QString errorMessage;
    QWeakPointer<QObject> instanceGuard;
    IPlugin *instance;
};


// PluginPath: Manages a plugin directory. It does nothing
// on construction following the "as lazy as possible" principle.
// In the "loaded" stage, it scans the directories and creates
// a list of PluginData for the libraries found.
// getInstances() will return the fully initialized list of
// IPlugins.

class PluginPath {
public:
    explicit PluginPath(const QDir &path);


    void getInstances(PluginManager::IPluginList *list);

    QDir path() const { return m_path; }

    //  Convenience to populate a "About Plugin" dialog with
    // plugins from that path. Forces initialization.
    QStandardItem *createModelItem();

private:
    typedef QList<PluginData> PluginDataList;

    static QStringList libraryFilePaths(const QDir &dir);
    void clear();
    void ensureLoaded();

    QDir m_path;
    bool m_loaded;
    PluginDataList m_plugins;
};
} // namespace QmlDesigner
#endif // PLUGINPATH_H
