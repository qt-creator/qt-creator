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

#ifndef PLUGINPATH_H
#define PLUGINPATH_H

#include <QObject>
#include <QPointer>
#include <QList>
#include <QDir>

QT_BEGIN_NAMESPACE
class QString;
class QAbstractItemModel;
class QStandardItem;
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
    QPointer<QObject> instanceGuard;
    IPlugin *instance;
};


// PluginPath: Manages a plugin directory. It does nothing
// on construction following the "as lazy as possible" principle.
// In the "loaded" stage, it scans the directories and creates
// a list of PluginData for the libraries found.
// getInstances() will return the fully initialized list of
// IPlugins.

class PluginPath {

    typedef QList<IPlugin *> IPluginList;
public:
    explicit PluginPath(const QDir &path);


    void getInstances(IPluginList *list);

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
