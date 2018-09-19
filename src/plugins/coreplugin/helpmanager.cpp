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

#include "helpmanager_implementation.h"

#include "coreplugin.h"

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
    auto plugin = Internal::CorePlugin::instance();
    // HelpManager API can only be used after the actual implementation has been created by the
    // Help plugin, so check that the plugins have all been created. That is the case
    // when the Core plugin is initialized.
    QTC_CHECK(plugin && plugin->pluginSpec()
              && plugin->pluginSpec()->state() >= ExtensionSystem::PluginSpec::Initialized);
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

void unregisterDocumentation(const QStringList &nameSpaces)
{
    if (checkInstance())
        m_instance->unregisterDocumentation(nameSpaces);
}

QMap<QString, QUrl> linksForIdentifier(const QString &id)
{
    return checkInstance() ? m_instance->linksForIdentifier(id) : QMap<QString, QUrl>();
}

QByteArray fileData(const QUrl &url)
{
    return checkInstance() ? m_instance->fileData(url) : QByteArray();
}

void handleHelpRequest(const QUrl &url, HelpManager::HelpViewerLocation location)
{
    if (checkInstance())
        m_instance->handleHelpRequest(url, location);
}

void handleHelpRequest(const QString &url, HelpViewerLocation location)
{
    handleHelpRequest(QUrl(url), location);
}

} // HelpManager
} // Core
