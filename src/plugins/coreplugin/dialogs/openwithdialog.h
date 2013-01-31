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

#ifndef OPENWITHDIALOG_H
#define OPENWITHDIALOG_H

#include <QDialog>
#include "ui_openwithdialog.h"

namespace Core {

class ICore;

namespace Internal {

// Present the user with a file name and a list of available
// editor kinds to choose from.
class OpenWithDialog : public QDialog, public Ui::OpenWithDialog
{
    Q_OBJECT

public:
    OpenWithDialog(const QString &fileName, QWidget *parent);

    void setEditors(const QStringList &);
    int editor() const;

    void setCurrentEditor(int index);

private slots:
    void currentItemChanged(QListWidgetItem *, QListWidgetItem *);

private:
    void setOkButtonEnabled(bool);
};

} // namespace Internal
} // namespace Core

#endif // OPENWITHDIALOG_H
