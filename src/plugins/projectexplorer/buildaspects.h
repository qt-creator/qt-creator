/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "projectexplorer_export.h"
#include "projectconfigurationaspects.h"

namespace Utils { class FilePath; }

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT BuildDirectoryAspect : public BaseStringAspect
{
    Q_OBJECT
public:
    BuildDirectoryAspect();
    ~BuildDirectoryAspect() override;

    void allowInSourceBuilds(const Utils::FilePath &sourceDir);
    bool isShadowBuild() const;
    void setProblem(const QString &description);

    void addToLayout(LayoutBuilder &builder) override;

private:
    void toMap(QVariantMap &map) const override;
    void fromMap(const QVariantMap &map) override;

    void updateProblemLabel();

    class Private;
    Private * const d;
};

class PROJECTEXPLORER_EXPORT SeparateDebugInfoAspect : public BaseTriStateAspect
{
    Q_OBJECT
public:
    SeparateDebugInfoAspect();
};

} // namespace ProjectExplorer
