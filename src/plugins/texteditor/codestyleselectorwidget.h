// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

protected:
    ICodeStylePreferences *m_codeStyle = nullptr;

private:
    void slotComboBoxActivated(int index);
    void slotCurrentDelegateChanged(ICodeStylePreferences *delegate);
    void slotCopyClicked();
    void slotRemoveClicked();
    virtual void slotImportClicked();
    virtual void slotExportClicked();
    void slotCodeStyleAdded(ICodeStylePreferences *codeStylePreferences);
    void slotCodeStyleRemoved(ICodeStylePreferences *codeStylePreferences);
    void slotUpdateName(ICodeStylePreferences *codeStylePreferences);

    void updateName(ICodeStylePreferences *codeStyle);
    ICodeStylePreferencesFactory *m_factory;
    ProjectExplorer::Project *m_project = nullptr;

    QString displayName(ICodeStylePreferences *codeStyle) const;

    Utils::Guard m_ignoreChanges;

    QComboBox *m_delegateComboBox;
    QPushButton *m_removeButton;
    QPushButton *m_exportButton;
    QPushButton *m_importButton;
};

} // namespace TextEditor
