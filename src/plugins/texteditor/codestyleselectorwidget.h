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

#include "texteditor_global.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QHBoxLayout;
class QComboBox;
class QLabel;
class QCheckBox;
class QPushButton;
QT_END_NAMESPACE

namespace TextEditor {

namespace Internal { namespace Ui { class CodeStyleSelectorWidget; } }

class ICodeStylePreferences;
class ICodeStylePreferencesFactory;

class TEXTEDITOR_EXPORT CodeStyleSelectorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CodeStyleSelectorWidget(ICodeStylePreferencesFactory *factory, QWidget *parent = 0);
    ~CodeStyleSelectorWidget();

    void setCodeStyle(TextEditor::ICodeStylePreferences *codeStyle);

private:
    void slotComboBoxActivated(int index);
    void slotCurrentDelegateChanged(TextEditor::ICodeStylePreferences *delegate);
    void slotCopyClicked();
    void slotEditClicked();
    void slotRemoveClicked();
    void slotImportClicked();
    void slotExportClicked();
    void slotCodeStyleAdded(ICodeStylePreferences*);
    void slotCodeStyleRemoved(ICodeStylePreferences*);
    void slotUpdateName();

    void updateName(ICodeStylePreferences *codeStyle);
    ICodeStylePreferencesFactory *m_factory;
    ICodeStylePreferences *m_codeStyle;

    QString displayName(ICodeStylePreferences *codeStyle) const;

    Internal::Ui::CodeStyleSelectorWidget *m_ui;

    bool m_ignoreGuiSignals;
};

} // namespace TextEditor
