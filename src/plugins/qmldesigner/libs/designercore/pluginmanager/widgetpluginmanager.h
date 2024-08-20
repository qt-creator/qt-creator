// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QList>

#include "widgetpluginpath.h"


QT_BEGIN_NAMESPACE
class QString;
class QAbstractItemModel;
QT_END_NAMESPACE

namespace QmlDesigner {

class IWidgetPlugin;

namespace Internal {

// PluginManager: Loads the plugin libraries on demand "as lazy as
// possible", that is, directories are scanned and
// instances are created only when  instances() is called.

class WidgetPluginManager
{
    Q_DISABLE_COPY(WidgetPluginManager)
    using PluginPathList = QList<WidgetPluginPath>;
public:
    using IWidgetPluginList = QList<IWidgetPlugin *>;

    WidgetPluginManager();

    bool addPath(const QString &path);

    IWidgetPluginList instances();

    // Convenience to create a model for an "About Plugins"
    // dialog. Forces plugin initialization.
    QAbstractItemModel *createModel(QObject *parent = nullptr);

private:
    PluginPathList m_paths;
};

} // namespace Internal
} // namespace QmlDesigner
