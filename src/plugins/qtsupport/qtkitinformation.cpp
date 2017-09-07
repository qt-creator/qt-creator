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

#include "qtkitinformation.h"

#include <QRegExp>

#include "qtkitconfigwidget.h"
#include "qtsupportconstants.h"
#include "qtversionmanager.h"
#include "qtparser.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>

#include <utils/algorithm.h>
#include <utils/buildablehelperlibrary.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport {

QtKitInformation::QtKitInformation()
{
    setObjectName(QLatin1String("QtKitInformation"));
    setId(QtKitInformation::id());
    setPriority(26000);

    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &QtKitInformation::kitsWereLoaded);
}

QVariant QtKitInformation::defaultValue(const Kit *k) const
{
    Q_UNUSED(k);

    // find "Qt in PATH":
    BaseQtVersion *result = QtVersionManager::version(equal(&BaseQtVersion::autodetectionSource,
                                                            QString::fromLatin1("PATH")));
    if (result)
        return result->uniqueId();

    // Use *any* desktop Qt:
    result = QtVersionManager::version(equal(&BaseQtVersion::type,
                                             QString::fromLatin1(QtSupport::Constants::DESKTOPQT)));
    return result ? result->uniqueId() : -1;
}

QList<ProjectExplorer::Task> QtKitInformation::validate(const ProjectExplorer::Kit *k) const
{
    QTC_ASSERT(QtVersionManager::isLoaded(), return { });
    BaseQtVersion *version = qtVersion(k);
    if (!version)
    return { };

    return version->validateKit(k);
}

void QtKitInformation::fix(ProjectExplorer::Kit *k)
{
    QTC_ASSERT(QtVersionManager::isLoaded(), return);
    BaseQtVersion *version = qtVersion(k);
    if (!version && qtVersionId(k) >= 0) {
        qWarning("Qt version is no longer known, removing from kit \"%s\".", qPrintable(k->displayName()));
        setQtVersionId(k, -1);
    }
}

ProjectExplorer::KitConfigWidget *QtKitInformation::createConfigWidget(ProjectExplorer::Kit *k) const
{
    return new Internal::QtKitConfigWidget(k, this);
}

QString QtKitInformation::displayNamePostfix(const ProjectExplorer::Kit *k) const
{
    BaseQtVersion *version = qtVersion(k);
    return version ? version->displayName() : QString();
}

ProjectExplorer::KitInformation::ItemList
QtKitInformation::toUserOutput(const ProjectExplorer::Kit *k) const
{
    BaseQtVersion *version = qtVersion(k);
    return ItemList() << qMakePair(tr("Qt version"), version ? version->displayName() : tr("None"));
}

void QtKitInformation::addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const
{
    BaseQtVersion *version = qtVersion(k);
    if (version)
        version->addToEnvironment(k, env);
}

ProjectExplorer::IOutputParser *QtKitInformation::createOutputParser(const ProjectExplorer::Kit *k) const
{
    if (qtVersion(k))
        return new QtParser;
    return 0;
}

void QtKitInformation::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    expander->registerSubProvider(
                [kit]() -> MacroExpander * {
                    BaseQtVersion *version = qtVersion(kit);
                    return version ? version->macroExpander() : 0;
                });

    expander->registerVariable("Qt:Name", tr("Name of Qt Version"),
                [kit]() -> QString {
                   BaseQtVersion *version = qtVersion(kit);
                   return version ? version->displayName() : tr("unknown");
                });
    expander->registerVariable("Qt:qmakeExecutable", tr("Path to the qmake executable"),
                [kit]() -> QString {
                    BaseQtVersion *version = qtVersion(kit);
                    return version ? version->qmakeCommand().toString() : QString();
                });
}

Core::Id QtKitInformation::id()
{
    return "QtSupport.QtInformation";
}

int QtKitInformation::qtVersionId(const ProjectExplorer::Kit *k)
{
    if (!k)
        return -1;

    int id = -1;
    QVariant data = k->value(QtKitInformation::id(), -1);
    if (data.type() == QVariant::Int) {
        bool ok;
        id = data.toInt(&ok);
        if (!ok)
            id = -1;
    } else {
        QString source = data.toString();
        BaseQtVersion *v = QtVersionManager::version([source](const BaseQtVersion *v) { return v->autodetectionSource() == source; });
        if (v)
            id = v->uniqueId();
    }
    return id;
}

void QtKitInformation::setQtVersionId(ProjectExplorer::Kit *k, const int id)
{
    k->setValue(QtKitInformation::id(), id);
}

BaseQtVersion *QtKitInformation::qtVersion(const ProjectExplorer::Kit *k)
{
    return QtVersionManager::version(qtVersionId(k));
}

void QtKitInformation::setQtVersion(ProjectExplorer::Kit *k, const BaseQtVersion *v)
{
    if (!v)
        setQtVersionId(k, -1);
    else
        setQtVersionId(k, v->uniqueId());
}

void QtKitInformation::qtVersionsChanged(const QList<int> &addedIds,
                                         const QList<int> &removedIds,
                                         const QList<int> &changedIds)
{
    Q_UNUSED(addedIds);
    Q_UNUSED(removedIds);
    foreach (ProjectExplorer::Kit *k, ProjectExplorer::KitManager::kits()) {
        if (changedIds.contains(qtVersionId(k))) {
            k->validate(); // Qt version may have become (in)valid
            notifyAboutUpdate(k);
        }
    }
}

void QtKitInformation::kitsWereLoaded()
{
    foreach (ProjectExplorer::Kit *k, ProjectExplorer::KitManager::kits())
        fix(k);

    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &QtKitInformation::qtVersionsChanged);
}

Kit::Predicate QtKitInformation::platformPredicate(Core::Id platform)
{
    return [platform](const Kit *kit) -> bool {
        BaseQtVersion *version = QtKitInformation::qtVersion(kit);
        return version && version->targetDeviceTypes().contains(platform);
    };
}

Kit::Predicate QtKitInformation::qtVersionPredicate(const QSet<Core::Id> &required,
                                                    const QtVersionNumber &min,
                                                    const QtVersionNumber &max)
{
    return [required, min, max](const Kit *kit) -> bool {
        BaseQtVersion *version = QtKitInformation::qtVersion(kit);
        if (!version)
            return false;
        QtVersionNumber current = version->qtVersion();
        if (min.majorVersion > -1 && current < min)
            return false;
        if (max.majorVersion > -1 && current > max)
            return false;
        return version->availableFeatures().contains(required);
    };
}

QSet<Core::Id> QtKitInformation::supportedPlatforms(const Kit *k) const
{
    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    return version ? version->targetDeviceTypes() : QSet<Core::Id>();
}

QSet<Core::Id> QtKitInformation::availableFeatures(const Kit *k) const
{
    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    return version ? version->availableFeatures() : QSet<Core::Id>();
}

} // namespace QtSupport
