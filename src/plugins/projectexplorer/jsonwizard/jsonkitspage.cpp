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

#include "jsonkitspage.h"
#include "jsonwizard.h"

#include "../iprojectmanager.h"
#include "../kit.h"
#include "../project.h"
#include "../projectexplorer.h"

#include <coreplugin/featureprovider.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/macroexpander.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

using namespace Core;

namespace ProjectExplorer {

const char KEY_FEATURE[] = "feature";
const char KEY_CONDITION[] = "condition";

JsonKitsPage::JsonKitsPage(QWidget *parent) : TargetSetupPage(parent)
{ }

void JsonKitsPage::initializePage()
{
    auto wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);

    connect(wiz, &JsonWizard::filesPolished, this, &JsonKitsPage::setupProjectFiles);

    const Id platform = Id::fromString(wiz->stringValue(QLatin1String("Platform")));
    const QSet<Id> preferred
            = evaluate(m_preferredFeatures, wiz->value(QLatin1String("PreferredFeatures")), wiz);
    const QSet<Id> required
            = evaluate(m_requiredFeatures, wiz->value(QLatin1String("RequiredFeatures")), wiz);

    setRequiredKitPredicate([required](const Kit *k) { return k->hasFeatures(required); });
    setPreferredKitPredicate([platform, preferred](const Kit *k) {
        return k->supportedPlatforms().contains(platform) && k->hasFeatures(preferred);
    });
    setProjectPath(wiz->expander()->expand(unexpandedProjectPath()));

    TargetSetupPage::initializePage();
}

void JsonKitsPage::cleanupPage()
{
    auto wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);

    disconnect(wiz, &JsonWizard::allDone, this, nullptr);

    TargetSetupPage::cleanupPage();
}

void JsonKitsPage::setUnexpandedProjectPath(const QString &path)
{
    m_unexpandedProjectPath = path;
}

QString JsonKitsPage::unexpandedProjectPath() const
{
    return m_unexpandedProjectPath;
}

void JsonKitsPage::setRequiredFeatures(const QVariant &data)
{
    m_requiredFeatures = parseFeatures(data);
}

void JsonKitsPage::setPreferredFeatures(const QVariant &data)
{
    m_preferredFeatures = parseFeatures(data);
}

void JsonKitsPage::setupProjectFiles(const JsonWizard::GeneratorFiles &files)
{
    Project *project = nullptr;
    QList<IProjectManager *> managerList = ExtensionSystem::PluginManager::getObjects<IProjectManager>();

    foreach (const JsonWizard::GeneratorFile &f, files) {
        if (f.file.attributes() & GeneratedFile::OpenProjectAttribute) {
            QString errorMessage;
            const QFileInfo fi(f.file.path());
            const QString path = fi.absoluteFilePath();

            Utils::MimeDatabase mdb;
            Utils::MimeType mt = mdb.mimeTypeForFile(fi);
            if (!mt.isValid())
                continue;

            auto manager = Utils::findOrDefault(managerList, Utils::equal(&IProjectManager::mimeType, mt.name()));
            project = manager ? manager->openProject(path, &errorMessage) : nullptr;
            if (project) {
                if (setupProject(project))
                    project->saveSettings();
                delete project;
                project = nullptr;
            }
        }
    }
}

QSet<Id> JsonKitsPage::evaluate(const QVector<JsonKitsPage::ConditionalFeature> &list,
                                const QVariant &defaultSet, JsonWizard *wiz)
{
    if (list.isEmpty())
        return Id::fromStringList(defaultSet.toStringList());

    QSet<Id> features;
    foreach (const ConditionalFeature &f, list) {
        if (JsonWizard::boolFromVariant(f.condition, wiz->expander()))
            features.insert(Id::fromString(wiz->expander()->expand(f.feature)));
    }
    return features;
}

QVector<JsonKitsPage::ConditionalFeature> JsonKitsPage::parseFeatures(const QVariant &data,
                                                                      QString *errorMessage)
{
    QVector<ConditionalFeature> result;
    if (errorMessage)
        errorMessage->clear();

    if (data.isNull())
        return result;
    if (data.type() != QVariant::List) {
        if (errorMessage)
            *errorMessage = tr("Feature list is set and not of type list.");
        return result;
    }

    foreach (const QVariant &element, data.toList()) {
        if (element.type() == QVariant::String) {
            result.append({ element.toString(), QVariant(true) });
        } else if (element.type() == QVariant::Map) {
            const QVariantMap obj = element.toMap();
            const QString feature = obj.value(QLatin1String(KEY_FEATURE)).toString();
            if (feature.isEmpty()) {
                if (errorMessage) {
                    *errorMessage = tr("No \"%1\" key found in feature list object.")
                        .arg(QLatin1String(KEY_FEATURE));
                }
                return QVector<ConditionalFeature>();
            }

            result.append({ feature, obj.value(QLatin1String(KEY_CONDITION), true) });
        } else {
            if (errorMessage)
                *errorMessage = tr("Feature list element is not a string or object.");
            return QVector<ConditionalFeature>();
        }
    }

    return result;
}

} // namespace ProjectExplorer
