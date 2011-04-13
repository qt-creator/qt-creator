/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef SUBMITMODEL_H
#define SUBMITMODEL_H

#include "vcsbase_global.h"

#include <QtGui/QStandardItemModel>

namespace VCSBase {

/* A 2-column (checkable, state, file name) model to be used to list the files-
 * in the submit editor. Provides header items and a convience to add files. */

class VCSBASE_EXPORT SubmitFileModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit SubmitFileModel(QObject *parent = 0);

    // Convenience to create and add rows containing a file plus status text.
    static QList<QStandardItem *> createFileRow(const QString &fileName, const QString &status = QString(), bool checked = true);
    QList<QStandardItem *> addFile(const QString &fileName, const QString &status = QString(), bool checked = true);

    // Find convenience that returns the whole row (as opposed to QStandardItemModel::find).
    QList<QStandardItem *> findRow(const QString &text, int column = 0) const;

    // Convenience to obtain a row
    QList<QStandardItem *> rowAt(int row) const;

    // Filter for entries contained in the filter list. Returns the
    // number of deleted entries.
    unsigned filter(const QStringList &filter, int column);
};

}

#endif // SUBMITMODEL_H
