// Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once
#include "coreplugin/dialogs/ioptionspage.h"
#include <QPointer>

namespace CppEditor {
namespace Internal {
class CppQuickFixSettingsWidget;

class CppQuickFixSettingsPage : public Core::IOptionsPage
{
public:
    CppQuickFixSettingsPage();

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    QPointer<CppQuickFixSettingsWidget> m_widget;
};

} // namespace Internal
} // namespace CppEditor
