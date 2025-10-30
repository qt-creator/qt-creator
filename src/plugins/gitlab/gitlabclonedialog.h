// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QPlainTextEdit;
class QPushButton;
QT_END_NAMESPACE

namespace Utils {
class FancyLineEdit;
class InfoLabel;
class PathChooser;
}

namespace GitLab {

class Project;

class GitLabCloneDialog : public QDialog
{
public:
    explicit GitLabCloneDialog(const Project &project, QWidget *parent = nullptr);
    ~GitLabCloneDialog();

private:
    void updateUi();
    void cloneProject();

    QComboBox *m_repositoryCB = nullptr;
    QCheckBox *m_submodulesCB = nullptr;
    QPushButton *m_cloneButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QPlainTextEdit *m_cloneOutput = nullptr;
    Utils::PathChooser *m_pathChooser = nullptr;
    Utils::FancyLineEdit *m_directoryLE = nullptr;
    Utils::InfoLabel *m_infoLabel = nullptr;
    QSingleTaskTreeRunner m_taskTreeRunner;
};

} // namespace GitLab
