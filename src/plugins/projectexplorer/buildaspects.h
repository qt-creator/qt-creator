// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/aspects.h>

namespace Utils { class FilePath; }

namespace ProjectExplorer {
class BuildConfiguration;

class PROJECTEXPLORER_EXPORT BuildDirectoryAspect : public Utils::StringAspect
{
    Q_OBJECT
public:
    explicit BuildDirectoryAspect(const BuildConfiguration *bc);
    ~BuildDirectoryAspect() override;

    void allowInSourceBuilds(const Utils::FilePath &sourceDir);
    bool isShadowBuild() const;
    void setProblem(const QString &description);

    void addToLayout(Utils::LayoutBuilder &builder) override;

    static Utils::FilePath fixupDir(const Utils::FilePath &dir);

private:
    void toMap(QVariantMap &map) const override;
    void fromMap(const QVariantMap &map) override;

    void updateProblemLabel();

    class Private;
    Private * const d;
};

class PROJECTEXPLORER_EXPORT SeparateDebugInfoAspect : public Utils::TriStateAspect
{
    Q_OBJECT
public:
    SeparateDebugInfoAspect();
};

} // namespace ProjectExplorer
