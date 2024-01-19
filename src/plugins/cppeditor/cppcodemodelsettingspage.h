// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppcodemodelsettings.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace CppEditor::Internal {

class CppCodeModelSettingsPage final : public Core::IOptionsPage
{
public:
    CppCodeModelSettingsPage();
};

class ClangdSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    ClangdSettingsWidget(const ClangdSettings::Data &settingsData, bool isForProject);
    ~ClangdSettingsWidget();

    ClangdSettings::Data settingsData() const;

signals:
    void settingsDataChanged();

private:
    class Private;
    Private * const d;
};

void setupClangdProjectSettingsPanel();
void setupClangdSettingsPage();

} // CppEditor::Internal
