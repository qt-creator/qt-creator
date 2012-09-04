/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QTSUPPORT_QTKITINFORMATION_H
#define QTSUPPORT_QTKITINFORMATION_H

#include "qtsupport_global.h"

#include <projectexplorer/kitinformation.h>

#include "baseqtversion.h"

namespace QtSupport {

class QTSUPPORT_EXPORT QtKitInformation : public ProjectExplorer::KitInformation
{
    Q_OBJECT

public:
    QtKitInformation();

    Core::Id dataId() const;

    unsigned int priority() const; // the higher the closer to the top.

    QVariant defaultValue(ProjectExplorer::Kit *p) const;

    QList<ProjectExplorer::Task> validate(ProjectExplorer::Kit *p) const;

    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *p) const;

    QString displayNamePostfix(const ProjectExplorer::Kit *p) const;

    ItemList toUserOutput(ProjectExplorer::Kit *p) const;

    void addToEnvironment(const ProjectExplorer::Kit *p, Utils::Environment &env) const;

    static int qtVersionId(const ProjectExplorer::Kit *p);
    static void setQtVersionId(ProjectExplorer::Kit *p, const int id);
    static BaseQtVersion *qtVersion(const ProjectExplorer::Kit *p);
    static void setQtVersion(ProjectExplorer::Kit *p, const BaseQtVersion *v);
};

class QTSUPPORT_EXPORT QtTypeKitMatcher : public ProjectExplorer::KitMatcher
{
public:
    QtTypeKitMatcher(const QString &type);

    bool matches(const ProjectExplorer::Kit *p) const;

private:
    QString m_type;
};

class QTSUPPORT_EXPORT QtPlatformKitMatcher : public ProjectExplorer::KitMatcher
{
public:
    QtPlatformKitMatcher(const QString &platform);

    bool matches(const ProjectExplorer::Kit *p) const;

private:
    QString m_platform;
};

class QTSUPPORT_EXPORT QtVersionKitMatcher : public ProjectExplorer::KitMatcher
{
public:
    explicit QtVersionKitMatcher(const Core::FeatureSet &required = Core::FeatureSet(),
                                     const QtVersionNumber &min = QtVersionNumber(0, 0, 0),
                                     const QtVersionNumber &max = QtVersionNumber(INT_MAX, INT_MAX, INT_MAX)) :
        m_min(min), m_max(max), m_features(required)
    { }

    bool matches(const ProjectExplorer::Kit *p) const;

private:
    QtVersionNumber m_min;
    QtVersionNumber m_max;
    Core::FeatureSet m_features;
};

} // namespace QtSupport

#endif // QTSUPPORT_QTKITINFORMATION_H
