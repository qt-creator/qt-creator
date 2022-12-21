// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/ipotentialkit.h>
#include <utils/detailswidget.h>

namespace Android {
namespace Internal {

class AndroidPotentialKit : public ProjectExplorer::IPotentialKit
{
    Q_OBJECT
public:
    QString displayName() const override;
    void executeFromMenu() override;
    QWidget *createWidget(QWidget *parent) const override;
    bool isEnabled() const override;
};

class AndroidPotentialKitWidget : public Utils::DetailsWidget
{
    Q_OBJECT
public:
    AndroidPotentialKitWidget(QWidget *parent);
private:
    void openOptions();
    void recheck();
};

}
}
