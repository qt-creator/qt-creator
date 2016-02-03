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
    ~CleanDialog() override;

    void setFileList(const QString &workingDirectory, const QStringList &files,
                     const QStringList &ignoredFiles);

public slots:
    void accept() override;

private:
    void slotDoubleClicked(const QModelIndex &);
    void selectAllItems(bool checked);
    void updateSelectAllCheckBox();

    QStringList checkedFiles() const;
    bool promptToDelete();
    void addFile(const QString &workingDirectory, QString fileName, bool checked);

    Internal::CleanDialogPrivate *const d;
};

} // namespace VcsBase
