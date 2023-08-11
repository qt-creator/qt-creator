// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/kitmanager.h>

namespace QmakeProjectManager {
namespace Internal {

class QmakeKitAspect
{
public:
    static Utils::Id id();
    enum class MkspecSource { User, Code };
    static void setMkspec(ProjectExplorer::Kit *k, const QString &mkspec, MkspecSource source);
    static QString mkspec(const ProjectExplorer::Kit *k);
    static QString effectiveMkspec(const ProjectExplorer::Kit *k);
    static QString defaultMkspec(const ProjectExplorer::Kit *k);
};

class QmakeKitAspectFactory : public ProjectExplorer::KitAspectFactory
{
public:
    QmakeKitAspectFactory();

    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *k) const override;

    ProjectExplorer::KitAspect *createKitAspect(ProjectExplorer::Kit *k) const override;

    ItemList toUserOutput(const ProjectExplorer::Kit *k) const override;

    void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const override;
};

} // namespace Internal
} // namespace QmakeProjectManager
