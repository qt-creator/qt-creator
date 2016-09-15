/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "scxmluifactory.h"
#include "genericscxmlplugin.h"
#include "isceditor.h"

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

    foreach (QFileInfo dllFileInfo, pluginDir.entryInfoList(nameFilters)) {

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
                qDebug() << tr("Created editor-instance.");
                m_plugins << scEditorInstance;
            } else {
                qWarning() << tr("Editor-instance is not of the type ISCEditor.");
                loader.unload();
            }
        }
    }

    // Last init plugins
    for (int i = 0; i < m_plugins.count(); ++i)
        m_plugins[i]->init(this);
}
