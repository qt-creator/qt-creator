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

#include "ui_codegensettingspagewidget.h"

#include "codegensettings.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

namespace QtSupport {
namespace Internal {

class CodeGenSettingsPageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CodeGenSettingsPageWidget(QWidget *parent = 0);

    CodeGenSettings parameters() const;
    void setParameters(const CodeGenSettings &p);

private:
    int uiEmbedding() const;
    void setUiEmbedding(int);

    Ui::CodeGenSettingsPageWidget m_ui;
};

class CodeGenSettingsPage : public Core::IOptionsPage
{
public:
    explicit CodeGenSettingsPage(QObject *parent = 0);

    QWidget *widget();
    void apply();
    void finish();

private:
    QPointer<CodeGenSettingsPageWidget> m_widget;
    CodeGenSettings m_parameters;
};

} // namespace Internal
} // namespace QtSupport
