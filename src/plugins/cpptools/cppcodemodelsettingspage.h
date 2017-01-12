/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "cppcodemodelsettings.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QSettings)

namespace CppTools {

class ClangDiagnosticConfigsWidget;

namespace Internal {

namespace Ui { class CppCodeModelSettingsPage; }

class CppCodeModelSettingsWidget: public QWidget
{
    Q_OBJECT

public:
    explicit CppCodeModelSettingsWidget(QWidget *parent = 0);
    ~CppCodeModelSettingsWidget();

    void setSettings(const QSharedPointer<CppCodeModelSettings> &s);
    void applyToSettings() const;

private:
    void setupGeneralWidgets();
    void setupClangCodeModelWidgets();

    bool applyGeneralWidgetsToSettings() const;
    bool applyClangCodeModelWidgetsToSettings() const;

private:
    Ui::CppCodeModelSettingsPage *m_ui;
    QPointer<ClangDiagnosticConfigsWidget> m_clangDiagnosticConfigsWidget;
    QSharedPointer<CppCodeModelSettings> m_settings;
};

class CppCodeModelSettingsPage: public Core::IOptionsPage
{
public:
    explicit CppCodeModelSettingsPage(QSharedPointer<CppCodeModelSettings> &settings,
                                      QObject *parent = 0);

    QWidget *widget();
    void apply();
    void finish();

private:
    const QSharedPointer<CppCodeModelSettings> m_settings;
    QPointer<CppCodeModelSettingsWidget> m_widget;
};

} // Internal namespace
} // CppTools namespace
