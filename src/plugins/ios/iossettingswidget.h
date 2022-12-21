// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "iosconfigurations.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QCoreApplication>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QPushButton;
class QTreeView;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace Ios::Internal {

class IosSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Ios::Internal::IosSettingsWidget)

public:
    IosSettingsWidget();
    ~IosSettingsWidget() final;

private:
    void apply() final;

    void saveSettings();

    void onStart();
    void onCreate();
    void onReset();
    void onRename();
    void onDelete();
    void onScreenshot();
    void onSelectionChanged();

private:
    Utils::PathChooser *m_pathWidget;
    QPushButton *m_startButton;
    QPushButton *m_renameButton;
    QPushButton *m_deleteButton;
    QPushButton *m_resetButton;
    QTreeView *m_deviceView;
    QCheckBox *m_deviceAskCheckBox;
};

} // Ios::Internal
