// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericscxmlplugin.h"
#include "isceditor.h"
#include "scxmleditortr.h"
#include "scxmluifactory.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QGenericPlugin>
#include <QGenericPluginFactory>
#include <QPluginLoader>

using namespace ScxmlEditor::PluginInterface;

ScxmlUiFactory::ScxmlUiFactory(QObject *parent)
    : QObject(parent)
{
    initPlugins();
}

ScxmlUiFactory::~ScxmlUiFactory()
{
    for (int i = m_plugins.count(); i--;)
        m_plugins[i]->detach();
}

void ScxmlUiFactory::documentChanged(DocumentChangeType type, ScxmlDocument *doc)
{
    for (int i = 0; i < m_plugins.count(); ++i)
        m_plugins[i]->documentChanged(type, doc);
}

void ScxmlUiFactory::refresh()
{
    for (int i = 0; i < m_plugins.count(); ++i)
        m_plugins[i]->refresh();
}

QObject *ScxmlUiFactory::object(const QString &type) const
{
    return m_objects.value(type, 0);
}

void ScxmlUiFactory::unregisterObject(const QString &type, QObject *obj)
{
    if (obj && m_objects[type] == obj)
        m_objects.take(type);
}

void ScxmlUiFactory::registerObject(const QString &type, QObject *obj)
{
    if (obj)
        m_objects[type] = obj;
}

bool ScxmlUiFactory::isActive(const QString &type, const QObject *obj) const
{
    return obj && m_objects.value(type, 0) == obj;
}

void ScxmlUiFactory::initPlugins()
{
    // First init general plugin
    m_plugins << new GenericScxmlPlugin;

    // Get additional plugins
    QDir pluginDir(QCoreApplication::applicationDirPath() + QDir::separator() + "SCEPlugins");

    QStringList nameFilters;
    nameFilters << "*.dll" << "*.so";

    const QList<QFileInfo> dllFileInfos = pluginDir.entryInfoList(nameFilters);
    for (QFileInfo dllFileInfo : dllFileInfos) {

        QPluginLoader loader(dllFileInfo.absoluteFilePath());
        loader.load();

        if (!loader.isLoaded())
            break;

        auto plugin = qobject_cast<QGenericPlugin*>(loader.instance());

        if (!plugin)
            break;

        QObject *instance = plugin->create(QString(), QString());

        if (instance) {
            auto scEditorInstance = qobject_cast<ISCEditor*>(instance);
            if (scEditorInstance) {
                qDebug() << Tr::tr("Created editor-instance.");
                m_plugins << scEditorInstance;
            } else {
                qWarning() << Tr::tr("Editor-instance is not of the type ISCEditor.");
                loader.unload();
            }
        }
    }

    // Last init plugins
    for (int i = 0; i < m_plugins.count(); ++i)
        m_plugins[i]->init(this);
}
