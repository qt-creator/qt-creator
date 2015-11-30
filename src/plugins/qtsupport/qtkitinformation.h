/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QTSUPPORT_QTKITINFORMATION_H
#define QTSUPPORT_QTKITINFORMATION_H

#include "qtsupport_global.h"

#include "baseqtversion.h"

#include <projectexplorer/kitinformation.h>

namespace Utils { class MacroExpander; }

namespace QtSupport {

class QTSUPPORT_EXPORT QtKitInformation : public ProjectExplorer::KitInformation
{
    Q_OBJECT

public:
    QtKitInformation();

    QVariant defaultValue(ProjectExplorer::Kit *k) const;

    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *k) const;
    void fix(ProjectExplorer::Kit *);

    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *k) const;

    QString displayNamePostfix(const ProjectExplorer::Kit *k) const;

    ItemList toUserOutput(const ProjectExplorer::Kit *k) const;

    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const;
    ProjectExplorer::IOutputParser *createOutputParser(const ProjectExplorer::Kit *k) const;
    void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const;

    static Core::Id id();
    static int qtVersionId(const ProjectExplorer::Kit *k);
    static void setQtVersionId(ProjectExplorer::Kit *k, const int id);
    static BaseQtVersion *qtVersion(const ProjectExplorer::Kit *k);
    static void setQtVersion(ProjectExplorer::Kit *k, const BaseQtVersion *v);

    static ProjectExplorer::KitMatcher platformMatcher(const QString &availablePlatforms);
    static ProjectExplorer::KitMatcher qtVersionMatcher(const Core::FeatureSet &required = Core::FeatureSet(),
                                 const QtVersionNumber &min = QtVersionNumber(0, 0, 0),
                                 const QtVersionNumber &max = QtVersionNumber(INT_MAX, INT_MAX, INT_MAX));

    QSet<QString> availablePlatforms(const ProjectExplorer::Kit *k) const;
    QString displayNameForPlatform(const ProjectExplorer::Kit *k, const QString &platform) const;
    Core::FeatureSet availableFeatures(const ProjectExplorer::Kit *k) const;

private slots:
    void qtVersionsChanged(const QList<int> &addedIds,
                           const QList<int> &removedIds,
                           const QList<int> &changedIds);
    void kitsWereLoaded();
};

} // namespace QtSupport

#endif // QTSUPPORT_QTKITINFORMATION_H
