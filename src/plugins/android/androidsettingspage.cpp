/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidsettingspage.h"

#include "androidsettingswidget.h"
#include "androidconstants.h"
#include "androidtoolchain.h"
#include <projectexplorer/toolchainmanager.h>

#include <QCoreApplication>

namespace Android {
namespace Internal {

AndroidSettingsPage::AndroidSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
    setId(Constants::ANDROID_SETTINGS_ID);
    setDisplayName(tr("Android Configurations"));
    setCategory(Constants::ANDROID_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Android",
        Constants::ANDROID_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::ANDROID_SETTINGS_CATEGORY_ICON));
}

bool AndroidSettingsPage::matches(const QString &searchKeyWord) const
{
    return m_keywords.contains(searchKeyWord, Qt::CaseInsensitive);
}

QWidget *AndroidSettingsPage::createPage(QWidget *parent)
{
    m_widget = new AndroidSettingsWidget(parent);
    if (m_keywords.isEmpty())
        m_keywords = m_widget->searchKeywords();
    return m_widget;
}

void AndroidSettingsPage::apply()
{
    m_widget->saveSettings();

    QList<ProjectExplorer::ToolChain *> existingToolChains = ProjectExplorer::ToolChainManager::instance()->toolChains();
    QList<ProjectExplorer::ToolChain *> toolchains = AndroidToolChainFactory::createToolChainsForNdk(AndroidConfigurations::instance().config().ndkLocation);
    foreach (ProjectExplorer::ToolChain *tc, toolchains) {
        bool found = false;
        for (int i = 0; i < existingToolChains.count(); ++i) {
            if (*(existingToolChains.at(i)) == *tc) {
                found = true;
                break;
            }
        }
        if (found)
            delete tc;
        else
            ProjectExplorer::ToolChainManager::instance()->registerToolChain(tc);
    }

    for (int i = 0; i < existingToolChains.count(); ++i) {
        ProjectExplorer::ToolChain *tc = existingToolChains.at(i);
        if (tc->type() == QLatin1String(Constants::ANDROID_TOOLCHAIN_TYPE)) {
            if (!tc->isValid()) {
                ProjectExplorer::ToolChainManager::instance()->deregisterToolChain(tc);
            }
        }
    }

    AndroidConfigurations::instance().updateAutomaticKitList();
}

void AndroidSettingsPage::finish()
{
}

} // namespace Internal
} // namespace Android
