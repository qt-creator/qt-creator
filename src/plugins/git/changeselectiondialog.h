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

#include <coreplugin/id.h>

#include <utils/fileutils.h>

#include <QDialog>
#include <QProcessEnvironment>

QT_BEGIN_NAMESPACE
class QPushButton;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProcess;
class QStringListModel;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

enum ChangeCommand {
    NoCommand,
    Checkout,
    CherryPick,
    Revert,
    Show
};

namespace Ui { class ChangeSelectionDialog; }

class ChangeSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    ChangeSelectionDialog(const QString &workingDirectory, Core::Id id, QWidget *parent);
    ~ChangeSelectionDialog() override;

    QString change() const;

    QString workingDirectory() const;
    ChangeCommand command() const;

private:
    void selectCommitFromRecentHistory();
    void setDetails(int exitCode);
    void recalculateCompletion();
    void recalculateDetails();
    void changeTextChanged(const QString &text);
    void acceptCheckout();
    void acceptCherryPick();
    void acceptRevert();
    void acceptShow();

    void enableButtons(bool b);
    void terminateProcess();

    Ui::ChangeSelectionDialog *m_ui;

    QProcess *m_process = nullptr;
    Utils::FileName m_gitExecutable;
    QProcessEnvironment m_gitEnvironment;
    ChangeCommand m_command = NoCommand;
    QStringListModel *m_changeModel = nullptr;
    QString m_oldWorkingDir;
};

} // namespace Internal
} // namespace Git
