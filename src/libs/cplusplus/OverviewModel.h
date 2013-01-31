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

#ifndef CPLUSPLUS_OVERVIEWMODEL_H
#define CPLUSPLUS_OVERVIEWMODEL_H

#include "CppDocument.h"
#include "Overview.h"
#include "Icons.h"

#include <QAbstractItemModel>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT OverviewModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Role {
        FileNameRole = Qt::UserRole + 1,
        LineNumberRole
    };

public:
    OverviewModel(QObject *parent = 0);
    virtual ~OverviewModel();

    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    Document::Ptr document() const;
    Symbol *symbolFromIndex(const QModelIndex &index) const;

public Q_SLOTS:
    void rebuild(CPlusPlus::Document::Ptr doc);

private:
    bool hasDocument() const;
    unsigned globalSymbolCount() const;
    Symbol *globalSymbolAt(unsigned index) const;

private:
    Document::Ptr _cppDocument;
    Overview _overview;
    Icons _icons;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_OVERVIEWMODEL_H
