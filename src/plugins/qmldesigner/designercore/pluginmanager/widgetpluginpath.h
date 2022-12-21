// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QPointer>
#include <QList>
#include <QDir>
#include <QStandardItem>

QT_BEGIN_NAMESPACE
class QString;
class QAbstractItemModel;
QT_END_NAMESPACE

namespace QmlDesigner {
class IWidgetPlugin;
namespace Internal {

// Dumb plugin data structure. Note that whereas QObjects can
// casted to an interface, QPointer does not work with the
// interface class, so, we need a separate QPointer as a guard
// to detect the deletion of a plugin instance which can happen
// in theory.
struct WidgetPluginData {
    WidgetPluginData(const QString &p = QString());

    QString path;
    bool failed;
    QString errorMessage;
    QPointer<QObject> instanceGuard;
    IWidgetPlugin *instance;
};


// PluginPath: Manages a plugin directory. It does nothing
// on construction following the "as lazy as possible" principle.
// In the "loaded" stage, it scans the directories and creates
// a list of PluginData for the libraries found.
// getInstances() will return the fully initialized list of
// IPlugins.

class WidgetPluginPath {
    using IWidgetPluginList = QList<IWidgetPlugin *>;
public:
    explicit WidgetPluginPath(const QDir &path);


    void getInstances(IWidgetPluginList *list);

    QDir path() const { return m_path; }

    //  Convenience to populate a "About Plugin" dialog with
    // plugins from that path. Forces initialization.
    QStandardItem *createModelItem();

private:
    using PluginDataList = QList<WidgetPluginData>;

    static QStringList libraryFilePaths(const QDir &dir);
    void clear();
    void ensureLoaded();

    QDir m_path;
    bool m_loaded;
    PluginDataList m_plugins;
};

} // namespace Internal
} // namespace QmlDesigner
