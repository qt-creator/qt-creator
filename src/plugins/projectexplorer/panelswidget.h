// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "projectsettingswidget.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT PanelsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PanelsWidget(QWidget *parent = nullptr, bool addStretch = true);
    PanelsWidget(const QString &displayName, QWidget *widget);
    PanelsWidget(const QString &displayName, ProjectSettingsWidget *widget);
    ~PanelsWidget() override;

    void addPropertiesPanel(const QString &displayName);
    void addGlobalSettingsProperties(ProjectSettingsWidget *widget);
    void addWidget(QWidget *widget);

    static int constexpr PanelVMargin = 14;

private:
    QVBoxLayout *m_layout;
    QWidget *m_root;
};

} // namespace ProjectExplorer
