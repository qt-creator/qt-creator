// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include "baseqtversion.h"

#include <projectexplorer/kitinformation.h>

namespace Utils { class MacroExpander; }

namespace QtSupport {

class QTSUPPORT_EXPORT QtKitAspect : public ProjectExplorer::KitAspect
{
    Q_OBJECT

public:
    QtKitAspect();

    void setup(ProjectExplorer::Kit *k) override;

    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *k) const override;
    void fix(ProjectExplorer::Kit *) override;

    ProjectExplorer::KitAspectWidget *createConfigWidget(ProjectExplorer::Kit *k) const override;

    QString displayNamePostfix(const ProjectExplorer::Kit *k) const override;

    ItemList toUserOutput(const ProjectExplorer::Kit *k) const override;

    void addToBuildEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const override;
    QList<Utils::OutputLineParser *> createOutputParsers(const ProjectExplorer::Kit *k) const override;
    void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const override;

    static Utils::Id id();
    static int qtVersionId(const ProjectExplorer::Kit *k);
    static void setQtVersionId(ProjectExplorer::Kit *k, const int id);
    static QtVersion *qtVersion(const ProjectExplorer::Kit *k);
    static void setQtVersion(ProjectExplorer::Kit *k, const QtVersion *v);

    static void addHostBinariesToPath(const ProjectExplorer::Kit *k, Utils::Environment &env);

    static ProjectExplorer::Kit::Predicate platformPredicate(Utils::Id availablePlatforms);
    static ProjectExplorer::Kit::Predicate
    qtVersionPredicate(const QSet<Utils::Id> &required = QSet<Utils::Id>(),
                       const QVersionNumber &min = QVersionNumber(0, 0, 0),
                       const QVersionNumber &max = QVersionNumber(INT_MAX, INT_MAX, INT_MAX));

    QSet<Utils::Id> supportedPlatforms(const ProjectExplorer::Kit *k) const override;
    QSet<Utils::Id> availableFeatures(const ProjectExplorer::Kit *k) const override;

private:
    int weight(const ProjectExplorer::Kit *k) const override;

    void qtVersionsChanged(const QList<int> &addedIds,
                           const QList<int> &removedIds,
                           const QList<int> &changedIds);
    void kitsWereLoaded();
};

class QTSUPPORT_EXPORT SuppliesQtQuickImportPath
{
public:
    static Utils::Id id();
};

class QTSUPPORT_EXPORT KitQmlImportPath
{
public:
    static Utils::Id id();
};

class QTSUPPORT_EXPORT KitHasMergedHeaderPathsWithQmlImportPaths
{
public:
    static Utils::Id id();
};

} // namespace QtSupport
