/**************************************************************************
 * *
 ** This file is part of Qt Creator Instrumentation Tools
 **
 ** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
 **
 ** Contact: Nokia Corporation (qt-info@nokia.com)
 **
 ** Commercial Usage
 **
 ** Licensees holding valid Qt Commercial licenses may use this file in
 ** accordance with the Qt Commercial License Agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and Nokia.
 **
 ** GNU Lesser General Public License Usage
 **
 ** Alternatively, this file may be used under the terms of the GNU Lesser
 ** General Public License version 2.1 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.LGPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU Lesser General Public License version 2.1 requirements
 ** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 **
 ** If you are unsure which license is appropriate for your use, please
 ** contact the sales department at http://qt.nokia.com/contact.
 **
 **************************************************************************/

#ifndef STARTREMOTEDIALOG_H
#define STARTREMOTEDIALOG_H

#include <QtGui/QDialog>

#include <utils/ssh/sshconnection.h>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

namespace Analyzer {

namespace Ui {
class StartRemoteDialog;
}

class StartRemoteDialog : public QDialog {
    Q_OBJECT

public:
    explicit StartRemoteDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
    virtual ~StartRemoteDialog();

    Utils::SshConnectionParameters sshParams() const;
    QString executable() const;
    QString arguments() const;
    QString workingDirectory() const;

private slots:
    void validate();
    virtual void accept();

private:
    Ui::StartRemoteDialog *m_ui;
};

}

#endif // STARTREMOTEDIALOG_H
