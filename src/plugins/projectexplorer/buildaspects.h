// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/aspects.h>

namespace ProjectExplorer {

class BuildConfiguration;

class PROJECTEXPLORER_EXPORT BuildDirectoryAspect : public Utils::FilePathAspect
{
    Q_OBJECT

public:
    explicit BuildDirectoryAspect(Utils::AspectContainer *container, const BuildConfiguration *bc);
    ~BuildDirectoryAspect() override;

    void allowInSourceBuilds(const Utils::FilePath &sourceDir);
    bool isShadowBuild() const;
    void setProblem(const QString &description);

    void addToLayout(Layouting::LayoutItem &parent) override;

    static Utils::FilePath fixupDir(const Utils::FilePath &dir);

private:
    void toMap(Utils::Store &map) const override;
    void fromMap(const Utils::Store &map) override;

    void updateProblemLabel();

    class Private;
    Private * const d;
};

class PROJECTEXPLORER_EXPORT SeparateDebugInfoAspect : public Utils::TriStateAspect
{
    Q_OBJECT
public:
    SeparateDebugInfoAspect(Utils::AspectContainer *container = nullptr);
};

} // namespace ProjectExplorer
