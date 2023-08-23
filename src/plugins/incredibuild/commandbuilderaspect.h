// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

#include <utils/aspects.h>

#include <QLabel>
#include <QPointer>

namespace IncrediBuild::Internal {

class CommandBuilderAspect final : public Utils::BaseAspect
{
    Q_OBJECT

public:
    explicit CommandBuilderAspect(ProjectExplorer::BuildStep *step);
    ~CommandBuilderAspect() final;

    QString fullCommandFlag(bool keepJobNum) const;

private:
    void addToLayout(Layouting::LayoutItem &parent) final;
    void fromMap(const Utils::Store &map) final;
    void toMap(Utils::Store &map) const final;

    void updateGui();

    class CommandBuilderAspectPrivate *d = nullptr;
};

} // IncrediBuild::Internal
