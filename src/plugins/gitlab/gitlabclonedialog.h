/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <QCoreApplication>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QPlainTextEdit;
class QPushButton;
QT_END_NAMESPACE

namespace Core { class ShellCommand; }

namespace Utils {
class FancyLineEdit;
class InfoLabel;
class PathChooser;
}

namespace GitLab {

class Project;

class GitLabCloneDialog : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(GitLab::GitLabCloneDialog)
public:
    explicit GitLabCloneDialog(const Project &project, QWidget *parent = nullptr);

private:
    void updateUi();
    void cloneProject();
    void cancel();
    void cloneFinished(bool ok, int exitCode);

    QComboBox * m_repositoryCB = nullptr;
    QCheckBox *m_submodulesCB = nullptr;
    QPushButton *m_cloneButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QPlainTextEdit *m_cloneOutput = nullptr;
    Utils::PathChooser *m_pathChooser = nullptr;
    Utils::FancyLineEdit *m_directoryLE = nullptr;
    Utils::InfoLabel *m_infoLabel = nullptr;
    Core::ShellCommand *m_command = nullptr;
    bool m_commandRunning = false;
};

} // namespace GitLab
