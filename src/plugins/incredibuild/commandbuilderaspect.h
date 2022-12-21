// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

#include <utils/aspects.h>

#include <QLabel>
#include <QPointer>

namespace IncrediBuild {
namespace Internal {

class CommandBuilderAspect final : public Utils::BaseAspect
{
    Q_OBJECT

public:
    explicit CommandBuilderAspect(ProjectExplorer::BuildStep *step);
    ~CommandBuilderAspect() final;

    QString fullCommandFlag(bool keepJobNum) const;

private:
    void addToLayout(Utils::LayoutBuilder &builder) final;
    void fromMap(const QVariantMap &map) final;
    void toMap(QVariantMap &map) const final;

    void updateGui();

    class CommandBuilderAspectPrivate *d = nullptr;
};

} // namespace Internal
} // namespace IncrediBuild
