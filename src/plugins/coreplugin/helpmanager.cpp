// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "helpmanager_implementation.h"

#include "coreplugin.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QUrl>

namespace Core {
namespace HelpManager {

// makes sure that plugins can connect to HelpManager signals even if the Help plugin is not loaded
Q_GLOBAL_STATIC(Signals, m_signals)

static Implementation *m_instance = nullptr;

static bool checkInstance()
{
    static bool afterPluginCreation = false;
    if (!afterPluginCreation) {
        using namespace ExtensionSystem;
        auto plugin = Internal::CorePlugin::instance();
        // HelpManager API can only be used after the actual implementation has been created by the
        // Help plugin, so check that the plugins have all been created. That is the case
        // when the Core plugin is initialized.
        PluginSpec *pluginSpec = PluginManager::specForPlugin(plugin);
        afterPluginCreation = (plugin && pluginSpec && pluginSpec->state() >= PluginSpec::Initialized);
        QTC_CHECK(afterPluginCreation);
    }
    return m_instance != nullptr;
}

Signals *Signals::instance()
{
    return m_signals;
}

Implementation::Implementation()
{
    QTC_CHECK(!m_instance);
    m_instance = this;
}

Implementation::~Implementation()
{
    m_instance = nullptr;
}

QString documentationPath()
{
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + '/' + RELATIVE_DOC_PATH);
}

void registerDocumentation(const QStringList &files)
{
    if (checkInstance())
        m_instance->registerDocumentation(files);
}

void unregisterDocumentation(const QStringList &fileNames)
{
    if (checkInstance())
        m_instance->unregisterDocumentation(fileNames);
}

QMultiMap<QString, QUrl> linksForIdentifier(const QString &id)
{
    return checkInstance() ? m_instance->linksForIdentifier(id) : QMultiMap<QString, QUrl>();
}

QMultiMap<QString, QUrl> linksForKeyword(const QString &keyword)
{
    return checkInstance() ? m_instance->linksForKeyword(keyword) : QMultiMap<QString, QUrl>();
}

QByteArray fileData(const QUrl &url)
{
    return checkInstance() ? m_instance->fileData(url) : QByteArray();
}

void showHelpUrl(const QUrl &url, HelpManager::HelpViewerLocation location)
{
    if (checkInstance())
        m_instance->showHelpUrl(url, location);
}

void showHelpUrl(const QString &url, HelpViewerLocation location)
{
    showHelpUrl(QUrl(url), location);
}

} // HelpManager
} // Core
