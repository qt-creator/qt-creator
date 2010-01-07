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

#ifndef WIDGETPLUGINPATH_H
#define WIDGETPLUGINPATH_H

#include "widgetpluginmanager.h"

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
class IWidgetPlugin;
namespace Internal {



// Dumb plugin data structure. Note that whereas QObjects can
// casted to an interface, QWeakPointer does not work with the
// interface class, so, we need a separate QWeakPointer as a guard
// to detect the deletion of a plugin instance which can happen
// in theory.
struct WidgetPluginData {
    WidgetPluginData(const QString &p = QString());

    QString path;
    bool failed;
    QString errorMessage;
    QWeakPointer<QObject> instanceGuard;
    IWidgetPlugin *instance;
};


// PluginPath: Manages a plugin directory. It does nothing
// on construction following the "as lazy as possible" principle.
// In the "loaded" stage, it scans the directories and creates
// a list of PluginData for the libraries found.
// getInstances() will return the fully initialized list of
// IPlugins.

class WidgetPluginPath {
public:
    explicit WidgetPluginPath(const QDir &path);


    void getInstances(WidgetPluginManager::IWidgetPluginList *list);

    QDir path() const { return m_path; }

    //  Convenience to populate a "About Plugin" dialog with
    // plugins from that path. Forces initialization.
    QStandardItem *createModelItem();

private:
    typedef QList<WidgetPluginData> PluginDataList;

    static QStringList libraryFilePaths(const QDir &dir);
    void clear();
    void ensureLoaded();

    QDir m_path;
    bool m_loaded;
    PluginDataList m_plugins;
};

} // namespace Internal
} // namespace QmlDesigner
#endif // WIDGETPLUGINPATH_H
