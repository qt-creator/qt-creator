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
    explicit PanelsWidget(bool addStretch);

public:
    PanelsWidget(const QString &displayName, QWidget *widget, bool addStretch = true);
    PanelsWidget(const QString &displayName, ProjectSettingsWidget *widget);
    ~PanelsWidget() final;

    static int constexpr PanelVMargin = 14;

private:
    void addPropertiesPanel(const QString &displayName);
    void addGlobalSettingsProperties(ProjectSettingsWidget *widget);
    void addWidget(QWidget *widget);

    QVBoxLayout *m_layout;
    QWidget *m_root;
};

} // ProjectExplorer::Internal
