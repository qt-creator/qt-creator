/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SUBMITFILEMODEL_H
#define SUBMITFILEMODEL_H

#include "vcsbase_global.h"

#include <QStandardItemModel>

namespace VcsBase {

enum CheckMode
{
    Unchecked,
    Checked,
    Uncheckable
};

class VCSBASE_EXPORT SubmitFileModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit SubmitFileModel(QObject *parent = 0);

    // Convenience to create and add rows containing a file plus status text.
    QList<QStandardItem *> addFile(const QString &fileName, const QString &status = QString(),
                                   CheckMode checkMode = Checked, const QVariant &data = QVariant());

    QString state(int row) const;
    QString file(int row) const;
    bool isCheckable(int row) const;
    bool checked(int row) const;
    void setChecked(int row, bool check);
    void setAllChecked(bool check);
    QVariant extraData(int row) const;

    bool hasCheckedFiles() const;

    // Filter for entries contained in the filter list. Returns the
    // number of deleted entries.
    unsigned int filterFiles(const QStringList &filter);

    virtual void updateSelections(SubmitFileModel *source);
};

} // namespace VcsBase

#endif // SUBMITFILEMODEL_H
