/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

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
