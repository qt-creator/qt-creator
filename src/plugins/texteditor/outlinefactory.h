// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/inavigationwidgetfactory.h>

namespace TextEditor::Internal {

class OutlineFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    OutlineFactory();

    // from INavigationWidgetFactory
    Core::NavigationView createWidget() override;
    void saveSettings(Utils::QtcSettings *settings, int position, QWidget *widget) override;
    void restoreSettings(Utils::QtcSettings *settings, int position, QWidget *widget) override;

signals:
    void updateOutline();
};

} // TextEditor::Internal
