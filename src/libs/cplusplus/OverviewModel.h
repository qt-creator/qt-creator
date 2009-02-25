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

#ifndef CPLUSPLUS_OVERVIEWMODEL_H
#define CPLUSPLUS_OVERVIEWMODEL_H

#include "CppDocument.h"
#include "Overview.h"
#include "Icons.h"

#include <QAbstractItemModel>
#include <QIcon>

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

} // end of namespace CPlusPlus

#endif // CPLUSPLUS_OVERVIEWMODEL_H
