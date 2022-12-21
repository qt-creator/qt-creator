// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/pluginview.h>
#include <extensionsystem/pluginmanager.h>

#include <QWidget>
#include <QPushButton>

class PluginDialog : public QWidget
{
    Q_OBJECT

public:
    PluginDialog();

private:
    void updateButtons();
    void openDetails(ExtensionSystem::PluginSpec *spec = nullptr);
    void openErrorDetails();

    ExtensionSystem::PluginView *m_view;

    QPushButton *m_detailsButton;
    QPushButton *m_errorDetailsButton;
};
