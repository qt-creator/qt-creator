/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CHANGESELECTIONDIALOG_H
#define CHANGESELECTIONDIALOG_H

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
    ~ChangeSelectionDialog();

    QString change() const;

    QString workingDirectory() const;
    ChangeCommand command() const;

private slots:
    void chooseWorkingDirectory();
    void selectCommitFromRecentHistory();
    void setDetails(int exitCode);
    void recalculateCompletion();
    void recalculateDetails();
    void changeTextChanged(const QString &text);
    void acceptCheckout();
    void acceptCherryPick();
    void acceptRevert();
    void acceptShow();

private:
    void enableButtons(bool b);
    void terminateProcess();

    Ui::ChangeSelectionDialog *m_ui;

    QProcess *m_process;
    Utils::FileName m_gitExecutable;
    QProcessEnvironment m_gitEnvironment;
    ChangeCommand m_command;
    QStringListModel *m_changeModel;
    QString m_oldWorkingDir;
};

} // namespace Internal
} // namespace Git

#endif // CHANGESELECTIONDIALOG_H
