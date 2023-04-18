// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/ipotentialkit.h>

namespace Android::Internal {

class AndroidPotentialKit : public ProjectExplorer::IPotentialKit
{
public:
    QString displayName() const override;
    void executeFromMenu() override;
    QWidget *createWidget(QWidget *parent) const override;
    bool isEnabled() const override;
};

} // Android::Internal
