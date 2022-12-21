// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <QFontDatabase>
#include <QPointer>

namespace Help {
namespace Internal {

class GeneralSettingsPageWidget;

class GeneralSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    GeneralSettingsPage();

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    void setCurrentPage();
    void setBlankPage();
    void setDefaultPage();
    void importBookmarks();
    void exportBookmarks();

    void updateFontSizeSelector();
    void updateFontStyleSelector();
    void updateFontFamilySelector();
    void updateFont();
    int closestPointSizeIndex(int desiredPointSize) const;

    QFont m_font;
    int m_fontZoom = 100;
    QFontDatabase m_fontDatabase;

    QString m_homePage;
    int m_contextOption;

    int m_startOption;
    bool m_returnOnClose;
    bool m_scrollWheelZoomingEnabled;

    QPointer<GeneralSettingsPageWidget> m_widget;
};

    }   // Internal
}   // Help
