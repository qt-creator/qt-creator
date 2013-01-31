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

#ifndef QTSUPPORT_QTKITINFORMATION_H
#define QTSUPPORT_QTKITINFORMATION_H

#include "qtsupport_global.h"

#include "baseqtversion.h"

#include <projectexplorer/kitinformation.h>

namespace QtSupport {

class QTSUPPORT_EXPORT QtKitInformation : public ProjectExplorer::KitInformation
{
    Q_OBJECT

public:
    QtKitInformation();

    Core::Id dataId() const;

    unsigned int priority() const; // the higher the closer to the top.

    QVariant defaultValue(ProjectExplorer::Kit *k) const;

    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *k) const;
    void fix(ProjectExplorer::Kit *);

    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *k) const;

    QString displayNamePostfix(const ProjectExplorer::Kit *k) const;

    ItemList toUserOutput(ProjectExplorer::Kit *k) const;

    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const;
    ProjectExplorer::IOutputParser *createOutputParser(const ProjectExplorer::Kit *k) const;

    static int qtVersionId(const ProjectExplorer::Kit *k);
    static void setQtVersionId(ProjectExplorer::Kit *k, const int id);
    static BaseQtVersion *qtVersion(const ProjectExplorer::Kit *k);
    static void setQtVersion(ProjectExplorer::Kit *k, const BaseQtVersion *v);

private slots:
    void qtVersionsChanged(const QList<int> &addedIds,
                           const QList<int> &removedIds,
                           const QList<int> &changedIds);
};

class QTSUPPORT_EXPORT QtPlatformKitMatcher : public ProjectExplorer::KitMatcher
{
public:
    QtPlatformKitMatcher(const QString &platform);

    bool matches(const ProjectExplorer::Kit *k) const;

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

    bool matches(const ProjectExplorer::Kit *k) const;

private:
    QtVersionNumber m_min;
    QtVersionNumber m_max;
    Core::FeatureSet m_features;
};

} // namespace QtSupport

#endif // QTSUPPORT_QTKITINFORMATION_H
