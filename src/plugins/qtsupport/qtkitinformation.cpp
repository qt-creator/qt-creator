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

#include "qtkitinformation.h"

#include "qtkitconfigwidget.h"
#include "qtsupportconstants.h"
#include "qtversionmanager.h"
#include "qtparser.h"

#include <utils/buildablehelperlibrary.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace QtSupport {

QtKitInformation::QtKitInformation()
{
    setObjectName(QLatin1String("QtKitInformation"));
    setId(QtKitInformation::id());
    setPriority(26000);

    connect(ProjectExplorer::KitManager::instance(), SIGNAL(kitsLoaded()),
            this, SLOT(kitsWereLoaded()));
}

QVariant QtKitInformation::defaultValue(ProjectExplorer::Kit *k) const
{
    Q_UNUSED(k);

    // find "Qt in PATH":
    Utils::FileName qmake = Utils::BuildableHelperLibrary::findSystemQt(Utils::Environment::systemEnvironment());

    if (qmake.isEmpty())
        return -1;

    QList<BaseQtVersion *> versionList = QtVersionManager::versions();
    BaseQtVersion *fallBack = 0;
    foreach (BaseQtVersion *v, versionList) {
        if (qmake == v->qmakeCommand())
            return v->uniqueId();
        if (v->type() == QLatin1String(QtSupport::Constants::DESKTOPQT) && !fallBack)
            fallBack = v;
    }
    if (fallBack)
        return fallBack->uniqueId();

    return -1;
}

QList<ProjectExplorer::Task> QtKitInformation::validate(const ProjectExplorer::Kit *k) const
{
    QList<ProjectExplorer::Task> result;
    QTC_ASSERT(QtVersionManager::isLoaded(), return result);
    BaseQtVersion *version = qtVersion(k);
    if (!version)
        return result;
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
        foreach (BaseQtVersion *v, QtVersionManager::versions()) {
            if (v->autodetectionSource() != source)
                continue;
            id = v->uniqueId();
            break;
        }
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

    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(qtVersionsChanged(QList<int>,QList<int>,QList<int>)));
}

KitMatcher QtKitInformation::platformMatcher(const QString &platform)
{
    return std::function<bool(const Kit *)>([platform](const Kit *kit) -> bool {
        BaseQtVersion *version = QtKitInformation::qtVersion(kit);
        return version && version->platformName() == platform;
    });
}

KitMatcher QtKitInformation::qtVersionMatcher(const Core::FeatureSet &required,
    const QtVersionNumber &min, const QtVersionNumber &max)
{
    return std::function<bool(const Kit *)>([required, min, max](const Kit *kit) -> bool {
        BaseQtVersion *version = QtKitInformation::qtVersion(kit);
        if (!version)
            return false;
        QtVersionNumber current = version->qtVersion();
        if (min.majorVersion > -1 && current < min)
            return false;
        if (max.majorVersion > -1 && current > max)
            return false;
        return version->availableFeatures().contains(required);
    });
}

QSet<QString> QtKitInformation::availablePlatforms(const Kit *k) const
{
    QSet<QString> result;
    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    if (version)
        result.insert(version->platformName());
    return result;
}

QString QtKitInformation::displayNameForPlatform(const Kit *k, const QString &platform) const
{
    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    if (version && version->platformName() == platform)
        return version->platformDisplayName();
    return QString();
}

Core::FeatureSet QtKitInformation::availableFeatures(const Kit *k) const
{
    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    return version ? version->availableFeatures() : Core::FeatureSet();
}

} // namespace QtSupport
