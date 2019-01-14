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

#include "cppoverviewmodel.h"

#include <cplusplus/Icons.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Symbols.h>

#include <utils/linecolumn.h>
#include <utils/link.h>

using namespace CPlusPlus;
namespace CppTools {

QVariant SymbolItem::data(int /*column*/, int role) const
{
    if (!symbol && parent()) { // account for no symbol item
        switch (role) {
        case Qt::DisplayRole:
            if (parent()->childCount() > 1)
                return QString(QT_TRANSLATE_NOOP("CppTools::OverviewModel", "<Select Symbol>"));
            return QString(QT_TRANSLATE_NOOP("CppTools::OverviewModel", "<No Symbols>"));
        default:
            return QVariant();
        }
    }

    auto overviewModel = qobject_cast<const OverviewModel*>(model());
    if (!symbol || !overviewModel)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole: {
        QString name = overviewModel->_overview.prettyName(symbol->name());
        if (name.isEmpty())
            name = QLatin1String("anonymous");
        if (symbol->isObjCForwardClassDeclaration())
            name = QLatin1String("@class ") + name;
        if (symbol->isObjCForwardProtocolDeclaration() || symbol->isObjCProtocol())
            name = QLatin1String("@protocol ") + name;
        if (symbol->isObjCClass()) {
            ObjCClass *clazz = symbol->asObjCClass();
            if (clazz->isInterface())
                name = QLatin1String("@interface ") + name;
            else
                name = QLatin1String("@implementation ") + name;

            if (clazz->isCategory()) {
                name += QString(" (%1)").arg(overviewModel->_overview.prettyName(
                                                 clazz->categoryName()));
            }
        }
        if (symbol->isObjCPropertyDeclaration())
            name = QLatin1String("@property ") + name;
        // if symbol is a template we might change it now - so, use a copy instead as we're const
        Symbol *symbl = symbol;
        if (Template *t = symbl->asTemplate())
            if (Symbol *templateDeclaration = t->declaration()) {
                QStringList parameters;
                parameters.reserve(static_cast<int>(t->templateParameterCount()));
                for (unsigned i = 0; i < t->templateParameterCount(); ++i) {
                    parameters.append(overviewModel->_overview.prettyName(
                                          t->templateParameterAt(i)->name()));
                }
                name += QString("<%1>").arg(parameters.join(QLatin1String(", ")));
                symbl = templateDeclaration;
            }
        if (symbl->isObjCMethod()) {
            ObjCMethod *method = symbl->asObjCMethod();
            if (method->isStatic())
                name = QLatin1Char('+') + name;
            else
                name = QLatin1Char('-') + name;
        } else if (! symbl->isScope() || symbl->isFunction()) {
            QString type = overviewModel->_overview.prettyType(symbl->type());
            if (Function *f = symbl->type()->asFunctionType()) {
                name += type;
                type = overviewModel->_overview.prettyType(f->returnType());
            }
            if (! type.isEmpty())
                name += QLatin1String(": ") + type;
        }
        return name;
    }

    case Qt::EditRole: {
        QString name = overviewModel->_overview.prettyName(symbol->name());
        if (name.isEmpty())
            name = QLatin1String("anonymous");
        return name;
    }

    case Qt::DecorationRole:
        return Icons::iconForSymbol(symbol);

    case AbstractOverviewModel::FileNameRole:
            return QString::fromUtf8(symbol->fileName(), static_cast<int>(symbol->fileNameLength()));

    case AbstractOverviewModel::LineNumberRole:
            return symbol->line();

    default:
        return QVariant();
    } // switch
}


bool OverviewModel::hasDocument() const
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

Symbol *OverviewModel::symbolFromIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;
    auto item = static_cast<const SymbolItem*>(itemForIndex(index));
    return item ? item->symbol : nullptr;
}

void OverviewModel::rebuild(Document::Ptr doc)
{
    beginResetModel();
    _cppDocument = doc;
    auto root = new SymbolItem;
    buildTree(root, true);
    setRootItem(root);
    endResetModel();
}

bool OverviewModel::isGenerated(const QModelIndex &sourceIndex) const
{
    CPlusPlus::Symbol *symbol = symbolFromIndex(sourceIndex);
    return symbol && symbol->isGenerated();
}

Utils::Link OverviewModel::linkFromIndex(const QModelIndex &sourceIndex) const
{
    CPlusPlus::Symbol *symbol = symbolFromIndex(sourceIndex);
    if (!symbol)
        return {};

    return symbol->toLink();
}

Utils::LineColumn OverviewModel::lineColumnFromIndex(const QModelIndex &sourceIndex) const
{
    Utils::LineColumn lineColumn;
    CPlusPlus::Symbol *symbol = symbolFromIndex(sourceIndex);
    if (!symbol)
        return lineColumn;
    lineColumn.line = static_cast<int>(symbol->line());
    lineColumn.column = static_cast<int>(symbol->column());
    return lineColumn;
}

OverviewModel::Range OverviewModel::rangeFromIndex(const QModelIndex &sourceIndex) const
{
    Utils::LineColumn lineColumn = lineColumnFromIndex(sourceIndex);
    return std::make_pair(lineColumn, lineColumn);
}

void OverviewModel::buildTree(SymbolItem *root, bool isRoot)
{
    if (!root)
        return;

    if (isRoot) {
        unsigned rows = globalSymbolCount();
        for (unsigned row = 0; row < rows; ++row) {
            Symbol *symbol = globalSymbolAt(row);
            auto currentItem = new SymbolItem(symbol);
            buildTree(currentItem, false);
            root->appendChild(currentItem);
        }
        root->prependChild(new SymbolItem); // account for no symbol item
    } else {
        Symbol *symbol = root->symbol;
        if (Scope *scope = symbol->asScope()) {
            Scope::iterator it = scope->memberBegin();
            Scope::iterator end = scope->memberEnd();
            for ( ; it != end; ++it) {
                if (!((*it)->name()))
                    continue;
                if ((*it)->asArgument())
                    continue;
                auto currentItem = new SymbolItem(*it);
                buildTree(currentItem, false);
                root->appendChild(currentItem);
            }
        }
    }
}

} // namespace CppTools
