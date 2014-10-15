/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "jsonkitspage.h"
#include "jsonwizard.h"

#include "../iprojectmanager.h"
#include "../kit.h"
#include "../project.h"
#include "../projectexplorer.h"

#include <coreplugin/featureprovider.h>
#include <coreplugin/mimedatabase.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

namespace ProjectExplorer {

JsonKitsPage::JsonKitsPage(QWidget *parent) : TargetSetupPage(parent)
{ }

void ProjectExplorer::JsonKitsPage::initializePage()
{
    JsonWizard *wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);

    connect(wiz, &JsonWizard::filesReady, this, &JsonKitsPage::setupProjectFiles);

    const QString platform = wiz->value(QLatin1String("Platform")).toString();
    const Core::FeatureSet preferred = Core::FeatureSet::fromStringList(wiz->value(QLatin1String("PreferredFeatures")).toStringList());
    const Core::FeatureSet required = Core::FeatureSet::fromStringList(wiz->value(QLatin1String("RequiredFeatures")).toStringList());
    const QString path = Utils::expandMacros(m_projectFilePath, wiz->expander());

    setProjectPath(path);

    setRequiredKitMatcher(KitMatcher([required](const Kit *k) { return k->hasFeatures(required); }));
    setPreferredKitMatcher(KitMatcher([platform, preferred](const Kit *k) { return k->hasPlatform(platform) && k->hasFeatures(preferred); }));

    TargetSetupPage::initializePage();
}

void JsonKitsPage::cleanupPage()
{
    JsonWizard *wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);

    disconnect(wiz, &JsonWizard::filesReady, this, 0);

    TargetSetupPage::cleanupPage();
}

void JsonKitsPage::setupProjectFiles(const JsonWizard::GeneratorFiles &files)
{
    Project *project = 0;
    QList<IProjectManager *> managerList = ExtensionSystem::PluginManager::getObjects<IProjectManager>();

    foreach (const JsonWizard::GeneratorFile &f, files) {
        if (f.file.attributes() & Core::GeneratedFile::OpenProjectAttribute) {
            QString errorMessage;
            QString path = f.file.path();
            const QFileInfo fi(path);

            if (fi.exists())
                path = fi.canonicalFilePath();

            Core::MimeType mt = Core::MimeDatabase::findByFile(fi);
            if (mt.isNull())
                continue;

            foreach (IProjectManager *manager, managerList) {
                if (manager->mimeType() == mt.type()) {
                    project = manager->openProject(path, &errorMessage);
                    break;
                }
            }

            if (project) {
                bool success = setupProject(project);
                if (success)
                    project->saveSettings();
                delete project;
            }
        }
    }
}

} // namespace ProjectExplorer
