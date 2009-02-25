/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef OPENWITHDIALOG_H
#define OPENWITHDIALOG_H

#include <QtGui/QDialog>
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
    QString editor() const;

    void setCurrentEditor(int index);

private slots:
    void currentItemChanged(QListWidgetItem *, QListWidgetItem *);

private:
    void setOkButtonEnabled(bool);
};

} // namespace Internal
} // namespace Core

#endif // OPENWITHDIALOG_H
