/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CLEANDIALOG_H
#define CLEANDIALOG_H

#include "vcsbase_global.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace VcsBase {

namespace Internal { class CleanDialogPrivate; }

class VCSBASE_EXPORT CleanDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CleanDialog(QWidget *parent = 0);
    ~CleanDialog();

    void setFileList(const QString &workingDirectory, const QStringList &files, const QStringList &ignoredFiles);

public slots:
    void accept();

protected:
    void changeEvent(QEvent *e);

private slots:
    void slotDoubleClicked(const QModelIndex &);

private:
    QStringList checkedFiles() const;
    bool promptToDelete();
    void addFile(const QString &workingDirectory, QString fileName, bool checked);

    Internal::CleanDialogPrivate *const d;
};

} // namespace VcsBase

#endif // CLEANDIALOG_H
