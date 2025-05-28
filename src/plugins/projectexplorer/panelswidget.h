// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectsettingswidget.h"

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace ProjectExplorer::Internal {

class PanelsWidget final : public QWidget
{
public:
    PanelsWidget(QWidget *widget, bool addStretch = true);
    PanelsWidget(ProjectSettingsWidget *widget);
    ~PanelsWidget() final;

    static int constexpr PanelVMargin = 14;

private:
    explicit PanelsWidget(bool addStretch);

    void addGlobalSettingsProperties(ProjectSettingsWidget *widget);
    void addWidget(QWidget *widget);

    QVBoxLayout *m_layout;
    QWidget *m_root;
};

} // ProjectExplorer::Internal
