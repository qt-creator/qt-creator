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
class QtcProcess;
} // Utils

namespace Git {
namespace Internal {

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
    Q_OBJECT
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

    std::unique_ptr<Utils::QtcProcess> m_process;
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

} // namespace Internal
} // namespace Git
