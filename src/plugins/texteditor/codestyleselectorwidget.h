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

#include <utils/guard.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace TextEditor {

class ICodeStylePreferences;
class ICodeStylePreferencesFactory;

class TEXTEDITOR_EXPORT CodeStyleSelectorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CodeStyleSelectorWidget(ICodeStylePreferencesFactory *factory,
                                     ProjectExplorer::Project *project = nullptr,
                                     QWidget *parent = nullptr);
    ~CodeStyleSelectorWidget() override;

    void setCodeStyle(ICodeStylePreferences *codeStyle);

private:
    void slotComboBoxActivated(int index);
    void slotCurrentDelegateChanged(ICodeStylePreferences *delegate);
    void slotCopyClicked();
    void slotRemoveClicked();
    void slotImportClicked();
    void slotExportClicked();
    void slotCodeStyleAdded(ICodeStylePreferences *codeStylePreferences);
    void slotCodeStyleRemoved(ICodeStylePreferences *codeStylePreferences);
    void slotUpdateName(ICodeStylePreferences *codeStylePreferences);

    void updateName(ICodeStylePreferences *codeStyle);
    ICodeStylePreferencesFactory *m_factory;
    ICodeStylePreferences *m_codeStyle = nullptr;
    ProjectExplorer::Project *m_project = nullptr;

    QString displayName(ICodeStylePreferences *codeStyle) const;

    Utils::Guard m_ignoreChanges;

    QComboBox *m_delegateComboBox;
    QPushButton *m_removeButton;
    QPushButton *m_exportButton;
    QPushButton *m_importButton;
};

} // namespace TextEditor
