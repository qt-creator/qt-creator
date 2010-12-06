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

#include "project.h"

#include "editorconfiguration.h"
#include "environment.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectnodes.h"
#include "target.h"
#include "userfileaccessor.h"

#include <coreplugin/ifile.h>
#include <extensionsystem/pluginmanager.h>
#include <limits>
#include <utils/qtcassert.h>

namespace {
const char * const ACTIVE_TARGET_KEY("ProjectExplorer.Project.ActiveTarget");
const char * const TARGET_KEY_PREFIX("ProjectExplorer.Project.Target.");
const char * const TARGET_COUNT_KEY("ProjectExplorer.Project.TargetCount");

const char * const EDITOR_SETTINGS_KEY("ProjectExplorer.Project.EditorSettings");

} // namespace

namespace ProjectExplorer {
// -------------------------------------------------------------------------
// Project
// -------------------------------------------------------------------------

class ProjectPrivate {
public:
    ProjectPrivate();
    QSet<QString> m_supportedTargetIds;
    QList<Target *> m_targets;
    Target *m_activeTarget;
    EditorConfiguration *m_editorConfiguration;
};

ProjectPrivate::ProjectPrivate() :
    m_activeTarget(0),
    m_editorConfiguration(new EditorConfiguration())
{
}

Project::Project() : d(new ProjectPrivate)
{
}

Project::~Project()
{
    qDeleteAll(d->m_targets);
    delete d->m_editorConfiguration;
}

bool Project::hasActiveBuildSettings() const
{
    return activeTarget() &&
           activeTarget()->buildConfigurationFactory();
}

QString Project::makeUnique(const QString &preferredName, const QStringList &usedNames)
{
    if (!usedNames.contains(preferredName))
        return preferredName;
    int i = 2;
    QString tryName = preferredName + QString::number(i);
    while (usedNames.contains(tryName))
        tryName = preferredName + QString::number(++i);
    return tryName;
}

QSet<QString> Project::supportedTargetIds() const
{
    return d->m_supportedTargetIds;
}

QSet<QString> Project::possibleTargetIds() const
{
    QSet<QString> result(d->m_supportedTargetIds);
    foreach (ProjectExplorer::Target *t, targets())
        result.remove(t->id());

    return result;
}

bool Project::canAddTarget(const QString &id) const
{
    return possibleTargetIds().contains(id);
}

void Project::setSupportedTargetIds(const QSet<QString> &ids)
{
    if (ids == d->m_supportedTargetIds)
        return;

    d->m_supportedTargetIds = ids;
    emit supportedTargetIdsChanged();
}

void Project::changeEnvironment()
{
    Target *t(qobject_cast<Target *>(sender()));
    if (t == activeTarget())
        emit environmentChanged();
}


void Project::addTarget(Target *t)
{
    QTC_ASSERT(t && !d->m_targets.contains(t), return);
    QTC_ASSERT(!target(t->id()), return);
    Q_ASSERT(t->project() == this);

    // Check that we don't have a configuration with the same displayName
    QString targetDisplayName = t->displayName();
    QStringList displayNames;
    foreach (const Target *target, d->m_targets)
        displayNames << target->displayName();
    targetDisplayName = makeUnique(targetDisplayName, displayNames);
    t->setDefaultDisplayName(targetDisplayName);

    // add it
    d->m_targets.push_back(t);
    connect(t, SIGNAL(environmentChanged()),
            SLOT(changeEnvironment()));
    emit addedTarget(t);

    // check activeTarget:
    if (activeTarget() == 0)
        setActiveTarget(t);
}

void Project::removeTarget(Target *target)
{
    QTC_ASSERT(target && d->m_targets.contains(target), return);

    emit aboutToRemoveTarget(target);

    d->m_targets.removeOne(target);

    emit removedTarget(target);
    if (target == activeTarget()) {
        if (d->m_targets.isEmpty())
            setActiveTarget(0);
        else
            setActiveTarget(d->m_targets.at(0));
    }
    delete target;
}

QList<Target *> Project::targets() const
{
    return d->m_targets;
}

Target *Project::activeTarget() const
{
    return d->m_activeTarget;
}

void Project::setActiveTarget(Target *target)
{
    if ((!target && !d->m_targets.isEmpty()) ||
        (target && d->m_targets.contains(target) && d->m_activeTarget != target)) {
        d->m_activeTarget = target;
        emit activeTargetChanged(d->m_activeTarget);
        emit environmentChanged();
    }
}

Target *Project::target(const QString &id) const
{
    foreach (Target * target, d->m_targets) {
        if (target->id() == id)
            return target;
    }
    return 0;
}

void Project::saveSettings()
{
    UserFileAccessor accessor;
    accessor.saveSettings(this, toMap());
}

bool Project::restoreSettings()
{
    UserFileAccessor accessor;
    QVariantMap map(accessor.restoreSettings(this));
    return fromMap(map);
}

QList<BuildConfigWidget*> Project::subConfigWidgets()
{
    return QList<BuildConfigWidget*>();
}

QVariantMap Project::toMap() const
{
    const QList<Target *> ts = targets();

    QVariantMap map;
    map.insert(QLatin1String(ACTIVE_TARGET_KEY), ts.indexOf(d->m_activeTarget));
    map.insert(QLatin1String(TARGET_COUNT_KEY), ts.size());
    for (int i = 0; i < ts.size(); ++i)
        map.insert(QString::fromLatin1(TARGET_KEY_PREFIX) + QString::number(i), ts.at(i)->toMap());

    map.insert(QLatin1String(EDITOR_SETTINGS_KEY), d->m_editorConfiguration->toMap());

    return map;
}

QString Project::projectDirectory() const
{
    return projectDirectory(file()->fileName());
}

QString Project::projectDirectory(const QString &proFile)
{
    if (proFile.isEmpty())
        return QString();
    QFileInfo info(proFile);
    return info.absoluteDir().path();
}


bool Project::fromMap(const QVariantMap &map)
{
    if (map.contains(QLatin1String(EDITOR_SETTINGS_KEY))) {
        QVariantMap values(map.value(QLatin1String(EDITOR_SETTINGS_KEY)).toMap());
        d->m_editorConfiguration->fromMap(values);
    }

    bool ok;
    int maxI(map.value(QLatin1String(TARGET_COUNT_KEY), 0).toInt(&ok));
    if (!ok || maxI < 0)
        maxI = 0;
    int active(map.value(QLatin1String(ACTIVE_TARGET_KEY), 0).toInt(&ok));
    if (!ok || active < 0)
        active = 0;
    if (0 > active || maxI < active)
        active = 0;

    for (int i = 0; i < maxI; ++i) {
        const QString key(QString::fromLatin1(TARGET_KEY_PREFIX) + QString::number(i));
        if (!map.contains(key)) {
            qWarning() << key << "was not found in data.";
            return false;
        }
       QVariantMap targetMap = map.value(key).toMap();

        QList<ITargetFactory *> factories =
                ExtensionSystem::PluginManager::instance()->getObjects<ITargetFactory>();

        Target *t = 0;
        foreach (ITargetFactory *factory, factories) {
            if (factory->canRestore(this, targetMap)) {
                t = factory->restore(this, targetMap);
                break;
            }
        }

        if (!t) {
            qWarning() << "Restoration of a target failed! (Continuing)";
            continue;
        }
        addTarget(t);
        if (i == active)
            setActiveTarget(t);
    }
    return true;
}

EditorConfiguration *Project::editorConfiguration() const
{
    return d->m_editorConfiguration;
}

QString Project::generatedUiHeader(const QString & /* formFile */) const
{
    return QString();
}

} // namespace ProjectExplorer
