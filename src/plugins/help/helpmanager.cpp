/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "helpmanager.h"
#include "helpplugin.h"

#include <coreplugin/icore.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QUrl>

#include <QtHelp/QHelpEngineCore>

using namespace Help;
using namespace Help::Internal;

QHelpEngineCore* HelpManager::m_coreEngine = 0;

HelpManager::HelpManager(HelpPlugin* plugin)
    : m_plugin(plugin)
{
}

HelpManager::~HelpManager()
{
    delete m_coreEngine;
    m_coreEngine = 0;
}

void HelpManager::handleHelpRequest(const QString &url)
{
    m_plugin->handleHelpRequest(url);
}

void HelpManager::registerDocumentation(const QStringList &fileNames)
{
    if (m_plugin) {
        m_plugin->setFilesToRegister(fileNames);
        emit registerDocumentation();
    }
}

QString HelpManager::collectionFilePath()
{
    const QFileInfo fi(Core::ICore::instance()->settings()->fileName());
    const QDir directory(fi.absolutePath() + QLatin1String("/qtcreator"));
    if (!directory.exists())
        directory.mkpath(directory.absolutePath());
    return directory.absolutePath() + QLatin1String("/helpcollection.qhc");
}

QHelpEngineCore& HelpManager::helpEngineCore()
{
    if (!m_coreEngine) {
        m_coreEngine = new QHelpEngineCore(collectionFilePath());
        m_coreEngine->setAutoSaveFilter(false);
        m_coreEngine->setCurrentFilter(tr("Unfiltered"));
        m_coreEngine->setupData();

    }
    return *m_coreEngine;
}
