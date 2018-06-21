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

#pragma once

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

    QVariant defaultValue(const ProjectExplorer::Kit *k) const override;

    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *k) const override;
    void fix(ProjectExplorer::Kit *) override;

    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *k) const override;

    QString displayNamePostfix(const ProjectExplorer::Kit *k) const override;

    ItemList toUserOutput(const ProjectExplorer::Kit *k) const override;

    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const override;
    ProjectExplorer::IOutputParser *createOutputParser(const ProjectExplorer::Kit *k) const override;
    void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const override;

    static Core::Id id();
    static int qtVersionId(const ProjectExplorer::Kit *k);
    static void setQtVersionId(ProjectExplorer::Kit *k, const int id);
    static BaseQtVersion *qtVersion(const ProjectExplorer::Kit *k);
    static void setQtVersion(ProjectExplorer::Kit *k, const BaseQtVersion *v);

    static ProjectExplorer::Kit::Predicate platformPredicate(Core::Id availablePlatforms);
    static ProjectExplorer::Kit::Predicate
    qtVersionPredicate(const QSet<Core::Id> &required = QSet<Core::Id>(),
                       const QtVersionNumber &min = QtVersionNumber(0, 0, 0),
                       const QtVersionNumber &max = QtVersionNumber(INT_MAX, INT_MAX, INT_MAX));

    QSet<Core::Id> supportedPlatforms(const ProjectExplorer::Kit *k) const override;
    QSet<Core::Id> availableFeatures(const ProjectExplorer::Kit *k) const override;

private:
    void qtVersionsChanged(const QList<int> &addedIds,
                           const QList<int> &removedIds,
                           const QList<int> &changedIds);
    void kitsWereLoaded();
};

} // namespace QtSupport
