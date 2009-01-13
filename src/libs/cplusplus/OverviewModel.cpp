/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "OverviewModel.h"
#include "Overview.h"

#include <Scope.h>
#include <Semantic.h>
#include <Literals.h>
#include <Symbols.h>

#include <QFile>
#include <QtDebug>

using namespace CPlusPlus;

OverviewModel::OverviewModel(QObject *parent)
    : QAbstractItemModel(parent)
{ }

OverviewModel::~OverviewModel()
{ }

bool OverviewModel::hasDocument() const
{
    return _cppDocument;
}

Document::Ptr OverviewModel::document() const
{
    return _cppDocument;
}

unsigned OverviewModel::globalSymbolCount() const
{
    unsigned count = 0;
    if (_cppDocument)
        count += _cppDocument->globalSymbolCount();
    return count;
}

Symbol *OverviewModel::globalSymbolAt(unsigned index) const
{ return _cppDocument->globalSymbolAt(index); }

QModelIndex OverviewModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row == 0) // account for no symbol item
            return createIndex(row, column);
        Symbol *symbol = globalSymbolAt(row-1); // account for no symbol item
        return createIndex(row, column, symbol);
    } else {
        Symbol *parentSymbol = static_cast<Symbol *>(parent.internalPointer());
        Q_ASSERT(parentSymbol);

        ScopedSymbol *scopedSymbol = parentSymbol->asScopedSymbol();
        Q_ASSERT(scopedSymbol);

        Scope *scope = scopedSymbol->members();
        Q_ASSERT(scope);

        return createIndex(row, 0, scope->symbolAt(row));
    }
}

QModelIndex OverviewModel::parent(const QModelIndex &child) const
{
    Symbol *symbol = static_cast<Symbol *>(child.internalPointer());
    if (!symbol) // account for no symbol item
        return QModelIndex();

    if (Scope *scope = symbol->scope()) {
        Symbol *parentSymbol = scope->owner();
        if (parentSymbol && parentSymbol->scope()) {
            QModelIndex index;
            if (parentSymbol->scope() && parentSymbol->scope()->owner()
                    && parentSymbol->scope()->owner()->scope()) // the parent doesn't have a parent
                index = createIndex(parentSymbol->index(), 0, parentSymbol);
            else //+1 to account for no symbol item
                index = createIndex(parentSymbol->index() + 1, 0, parentSymbol);
            return index;
        }
    }

    return QModelIndex();
}

int OverviewModel::rowCount(const QModelIndex &parent) const
{
    if (hasDocument()) {
        if (!parent.isValid()) {
            return globalSymbolCount()+1; // account for no symbol item
        } else {
            if (!parent.parent().isValid() && parent.row() == 0) // account for no symbol item
                return 0;
            Symbol *parentSymbol = static_cast<Symbol *>(parent.internalPointer());
            Q_ASSERT(parentSymbol);

            if (ScopedSymbol *scopedSymbol = parentSymbol->asScopedSymbol()) {
                if (!scopedSymbol->isFunction()) {
                    Scope *parentScope = scopedSymbol->members();
                    Q_ASSERT(parentScope);

                    return parentScope->symbolCount();
                }
            }
            return 0;
        }
    }
    if (!parent.isValid())
        return 1; // account for no symbol item
    return 0;
}

int OverviewModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant OverviewModel::data(const QModelIndex &index, int role) const
{
    // account for no symbol item
    if (!index.parent().isValid() && index.row() == 0) {
        switch (role) {
        case Qt::DisplayRole:
            if (rowCount() > 1)
                return tr("<Select Symbol>");
            else
                return tr("<No Symbols>");
        default:
            return QVariant();
        } //switch
    }

    switch (role) {
    case Qt::DisplayRole: {
        Symbol *symbol = static_cast<Symbol *>(index.internalPointer());
        QString name = _overview.prettyName(symbol->name());
        if (name.isEmpty())
            name = QLatin1String("anonymous");
        if (! symbol->isScopedSymbol() || symbol->isFunction()) {
            QString type = _overview.prettyType(symbol->type());
            if (! type.isEmpty()) {
                if (! symbol->type()->isFunction())
                    name += QLatin1String(": ");
                name += type;
            }
        }
        return name;
    }

    case Qt::EditRole: {
        Symbol *symbol = static_cast<Symbol *>(index.internalPointer());
        QString name = _overview.prettyName(symbol->name());
        if (name.isEmpty())
            name = QLatin1String("anonymous");
        return name;
    }

    case Qt::DecorationRole: {
        Symbol *symbol = static_cast<Symbol *>(index.internalPointer());
        return _icons.iconForSymbol(symbol);
    } break;

    case FileNameRole: {
        Symbol *symbol = static_cast<Symbol *>(index.internalPointer());
        return QString::fromUtf8(symbol->fileName(), symbol->fileNameLength());
    }

    case LineNumberRole: {
        Symbol *symbol = static_cast<Symbol *>(index.internalPointer());
        return symbol->line();
    }

    default:
        return QVariant();
    } // switch
}

Symbol *OverviewModel::symbolFromIndex(const QModelIndex &index) const
{
    return static_cast<Symbol *>(index.internalPointer());
}

void OverviewModel::rebuild(Document::Ptr doc)
{
    _cppDocument = doc;
    reset();
}
