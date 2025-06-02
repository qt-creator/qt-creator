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
    explicit PanelsWidget(bool addStretch);
    ~PanelsWidget() final;

    static int constexpr PanelVMargin = 14;

    void addGlobalSettingsProperties(ProjectSettingsWidget *widget);
    void addWidget(QWidget *widget);

private:
    QVBoxLayout *m_layout;
    QWidget *m_root;
};

} // ProjectExplorer::Internal
