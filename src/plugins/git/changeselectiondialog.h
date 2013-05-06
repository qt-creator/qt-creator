/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CHANGESELECTIONDIALOG_H
#define CHANGESELECTIONDIALOG_H

#include <QDialog>
#include <QProcessEnvironment>

QT_BEGIN_NAMESPACE
class QPushButton;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProcess;
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

class ChangeSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    ChangeSelectionDialog(const QString &workingDirectory, QWidget *parent);
    ~ChangeSelectionDialog();

    QString change() const;

    QString workingDirectory() const;
    ChangeCommand command() const;

private slots:
    void chooseWorkingDirectory();
    void selectCommitFromRecentHistory();
    void setDetails(int exitCode);
    void recalculateDetails();
    void acceptCheckout();
    void acceptCherryPick();
    void acceptRevert();
    void acceptShow();

private:
    void enableButtons(bool b);

    QProcess *m_process;
    QString m_gitBinaryPath;
    QProcessEnvironment m_gitEnvironment;

    QLineEdit *m_workingDirEdit;
    QLineEdit *m_changeNumberEdit;
    QPushButton *m_selectDirButton;
    QPushButton *m_selectFromHistoryButton;
    QPushButton *m_showButton;
    QPushButton *m_cherryPickButton;
    QPushButton *m_revertButton;
    QPushButton *m_checkoutButton;
    QPushButton *m_closeButton;
    QPlainTextEdit *m_detailsText;

    ChangeCommand m_command;
};

} // namespace Internal
} // namespace Git

#endif // CHANGESELECTIONDIALOG_H
