/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <utils/environment.h>

namespace QtSupport {
namespace Internal {
const char QT_INFORMATION[] = "QtSupport.QtInformation";
} // namespace Internal

QtKitInformation::QtKitInformation()
{
    setObjectName(QLatin1String("QtKitInformation"));
    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SIGNAL(validationNeeded()));
    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(qtVersionsChanged(QList<int>,QList<int>,QList<int>)));
}

Core::Id QtKitInformation::dataId() const
{
    static Core::Id id = Core::Id(Internal::QT_INFORMATION);
    return id;
}

unsigned int QtKitInformation::priority() const
{
    return 26000;
}

QVariant QtKitInformation::defaultValue(ProjectExplorer::Kit *k) const
{
    Q_UNUSED(k);
    QtVersionManager *mgr = QtVersionManager::instance();

    // find "Qt in PATH":
    Utils::Environment env = Utils::Environment::systemEnvironment();
    Utils::FileName qmake = Utils::FileName::fromString(env.searchInPath(QLatin1String("qmake")));

    if (qmake.isEmpty())
        return -1;

    QList<BaseQtVersion *> versionList = mgr->versions();
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
    BaseQtVersion *version = qtVersion(k);
    if (!version)
        return QList<ProjectExplorer::Task>();
    return version->validateKit(k);
}

void QtKitInformation::fix(ProjectExplorer::Kit *k)
{
    BaseQtVersion *version = qtVersion(k);
    if (!version)
        setQtVersionId(k, -1);
}

ProjectExplorer::KitConfigWidget *QtKitInformation::createConfigWidget(ProjectExplorer::Kit *k) const
{
    return new Internal::QtKitConfigWidget(k);
}

QString QtKitInformation::displayNamePostfix(const ProjectExplorer::Kit *k) const
{
    BaseQtVersion *version = qtVersion(k);
    return version ? version->displayName() : QString();
}

ProjectExplorer::KitInformation::ItemList
QtKitInformation::toUserOutput(ProjectExplorer::Kit *k) const
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

int QtKitInformation::qtVersionId(const ProjectExplorer::Kit *k)
{
    if (!k)
        return -1;

    int id = -1;
    QVariant data = k->value(Core::Id(Internal::QT_INFORMATION), -1);
    if (data.type() == QVariant::Int) {
        bool ok;
        id = data.toInt(&ok);
        if (!ok)
            id = -1;
    } else {
        QString source = data.toString();
        foreach (BaseQtVersion *v, QtVersionManager::instance()->versions()) {
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
    k->setValue(Core::Id(Internal::QT_INFORMATION), id);
}

BaseQtVersion *QtKitInformation::qtVersion(const ProjectExplorer::Kit *k)
{
    return QtVersionManager::instance()->version(qtVersionId(k));
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
    foreach (ProjectExplorer::Kit *k, ProjectExplorer::KitManager::instance()->kits())
        if (changedIds.contains(qtVersionId(k)))
            notifyAboutUpdate(k);
}

QtPlatformKitMatcher::QtPlatformKitMatcher(const QString &platform) :
    m_platform(platform)
{ }

bool QtPlatformKitMatcher::matches(const ProjectExplorer::Kit *k) const
{
    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    return version && version->platformName() == m_platform;
}

bool QtVersionKitMatcher::matches(const ProjectExplorer::Kit *k) const
{
    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    if (!version)
        return false;
    QtVersionNumber current = version->qtVersion();
    if (m_min.majorVersion > -1 && current < m_min)
        return false;
    if (m_max.majorVersion > -1 && current > m_max)
        return false;
    return version->availableFeatures().contains(m_features);
}

} // namespace QtSupport
