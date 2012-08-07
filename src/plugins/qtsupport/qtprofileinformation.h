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

#ifndef QTSUPPORT_QTPROFILEINFORMATION_H
#define QTSUPPORT_QTPROFILEINFORMATION_H

#include "qtsupport_global.h"

#include <projectexplorer/profileinformation.h>

#include "baseqtversion.h"

namespace QtSupport {

class QTSUPPORT_EXPORT QtProfileInformation : public ProjectExplorer::ProfileInformation
{
    Q_OBJECT

public:
    QtProfileInformation();

    Core::Id dataId() const;

    unsigned int priority() const; // the higher the closer to the top.

    QVariant defaultValue(ProjectExplorer::Profile *p) const;

    QList<ProjectExplorer::Task> validate(ProjectExplorer::Profile *p) const;

    ProjectExplorer::ProfileConfigWidget *createConfigWidget(ProjectExplorer::Profile *p) const;

    QString displayNamePostfix(const ProjectExplorer::Profile *p) const;

    ItemList toUserOutput(ProjectExplorer::Profile *p) const;

    void addToEnvironment(const ProjectExplorer::Profile *p, Utils::Environment &env) const;

    static int qtVersionId(const ProjectExplorer::Profile *p);
    static void setQtVersionId(ProjectExplorer::Profile *p, const int id);
    static BaseQtVersion *qtVersion(const ProjectExplorer::Profile *p);
    static void setQtVersion(ProjectExplorer::Profile *p, const BaseQtVersion *v);
};

class QTSUPPORT_EXPORT QtTypeProfileMatcher : public ProjectExplorer::ProfileMatcher
{
public:
    QtTypeProfileMatcher(const QString &type);

    bool matches(const ProjectExplorer::Profile *p) const;

private:
    QString m_type;
};

class QTSUPPORT_EXPORT QtPlatformProfileMatcher : public ProjectExplorer::ProfileMatcher
{
public:
    QtPlatformProfileMatcher(const QString &platform);

    bool matches(const ProjectExplorer::Profile *p) const;

private:
    QString m_platform;
};

class QTSUPPORT_EXPORT QtVersionProfileMatcher : public ProjectExplorer::ProfileMatcher
{
public:
    explicit QtVersionProfileMatcher(const Core::FeatureSet &required = Core::FeatureSet(),
                                     const QtVersionNumber &min = QtVersionNumber(0, 0, 0),
                                     const QtVersionNumber &max = QtVersionNumber(INT_MAX, INT_MAX, INT_MAX)) :
        m_min(min), m_max(max), m_features(required)
    { }

    bool matches(const ProjectExplorer::Profile *p) const;

private:
    QtVersionNumber m_min;
    QtVersionNumber m_max;
    Core::FeatureSet m_features;
};

} // namespace QtSupport

#endif // QTSUPPORT_QTPROFILEINFORMATION_H
