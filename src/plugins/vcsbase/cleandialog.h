/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CLEANDIALOG_H
#define CLEANDIALOG_H

#include "vcsbase_global.h"

#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace VCSBase {
struct CleanDialogPrivate;

/* CleanDialog: Completely clean a directory under version control
 * from all files that are not under version control based on a list
 * generated from the version control system. Presents the user with
 * a checkable list of files and/or directories. Double click opens a file. */

class VCSBASE_EXPORT CleanDialog : public QDialog {
    Q_OBJECT
public:
    explicit CleanDialog(QWidget *parent = 0);
    virtual ~CleanDialog();

    void setFileList(const QString &workingDirectory, const QStringList &);

public slots:
    virtual void accept();

protected:
    void changeEvent(QEvent *e);

private slots:
    void slotDoubleClicked(const QModelIndex &);

private:
    QStringList checkedFiles() const;
    bool promptToDelete();

    CleanDialogPrivate *d;
};

} // namespace VCSBase
#endif // CLEANDIALOG_H
