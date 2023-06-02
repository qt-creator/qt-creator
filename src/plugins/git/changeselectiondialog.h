// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/id.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QPushButton;
class QStringListModel;
QT_END_NAMESPACE

namespace Utils {
class CompletingLineEdit;
class PathChooser;
class Process;
} // Utils

namespace Git::Internal {

enum ChangeCommand {
    NoCommand,
    Archive,
    Checkout,
    CherryPick,
    Revert,
    Show
};

class ChangeSelectionDialog : public QDialog
{
public:
    ChangeSelectionDialog(const Utils::FilePath &workingDirectory, Utils::Id id, QWidget *parent);
    ~ChangeSelectionDialog() override;

    QString change() const;

    Utils::FilePath workingDirectory() const;
    ChangeCommand command() const;

private:
    void selectCommitFromRecentHistory();
    void setDetails();
    void recalculateCompletion();
    void recalculateDetails();
    void changeTextChanged(const QString &text);
    void acceptCommand(ChangeCommand command);

    void enableButtons(bool b);

    std::unique_ptr<Utils::Process> m_process;
    Utils::FilePath m_gitExecutable;
    Utils::Environment m_gitEnvironment;
    ChangeCommand m_command = NoCommand;
    QStringListModel *m_changeModel = nullptr;
    Utils::FilePath m_oldWorkingDir;

    Utils::PathChooser *m_workingDirectoryChooser;
    Utils::CompletingLineEdit *m_changeNumberEdit;
    QPlainTextEdit *m_detailsText;
    QPushButton *m_checkoutButton;
    QPushButton *m_revertButton;
    QPushButton *m_cherryPickButton;
    QPushButton *m_showButton;
};

} // Git::Internal
