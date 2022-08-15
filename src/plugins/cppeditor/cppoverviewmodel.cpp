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
namespace CppEditor::Internal {

class SymbolItem : public Utils::TreeItem
{
public:
    SymbolItem() = default;
    explicit SymbolItem(CPlusPlus::Symbol *symbol) : symbol(symbol) {}

    QVariant data(int column, int role) const override
    {
        Q_UNUSED(column)

        if (!symbol && parent()) { // account for no symbol item
            switch (role) {
            case Qt::DisplayRole:
                if (parent()->childCount() > 1)
                    return QString(QT_TRANSLATE_NOOP("CppEditor::OverviewModel", "<Select Symbol>"));
                return QString(QT_TRANSLATE_NOOP("CppEditor::OverviewModel", "<No Symbols>"));
            default:
                return QVariant();
            }
        }

        auto overviewModel = qobject_cast<const OverviewModel*>(model());
        if (!symbol || !overviewModel)
            return QVariant();

        switch (role) {
        case Qt::DisplayRole: {
            QString name = overviewModel->m_overview.prettyName(symbol->name());
            if (name.isEmpty())
                name = QLatin1String("anonymous");
            if (symbol->asObjCForwardClassDeclaration())
                name = QLatin1String("@class ") + name;
            if (symbol->asObjCForwardProtocolDeclaration() || symbol->asObjCProtocol())
                name = QLatin1String("@protocol ") + name;
            if (symbol->asObjCClass()) {
                ObjCClass *clazz = symbol->asObjCClass();
                if (clazz->isInterface())
                    name = QLatin1String("@interface ") + name;
                else
                    name = QLatin1String("@implementation ") + name;

                if (clazz->isCategory()) {
                    name += QString(" (%1)").arg(overviewModel->m_overview.prettyName(
                                                     clazz->categoryName()));
                }
            }
            if (symbol->asObjCPropertyDeclaration())
                name = QLatin1String("@property ") + name;
            // if symbol is a template we might change it now - so, use a copy instead as we're const
            Symbol *symbl = symbol;
            if (Template *t = symbl->asTemplate())
                if (Symbol *templateDeclaration = t->declaration()) {
                    QStringList parameters;
                    parameters.reserve(t->templateParameterCount());
                    for (int i = 0; i < t->templateParameterCount(); ++i) {
                        parameters.append(overviewModel->m_overview.prettyName(
                                              t->templateParameterAt(i)->name()));
                    }
                    name += QString("<%1>").arg(parameters.join(QLatin1String(", ")));
                    symbl = templateDeclaration;
                }
            if (symbl->asObjCMethod()) {
                ObjCMethod *method = symbl->asObjCMethod();
                if (method->isStatic())
                    name = QLatin1Char('+') + name;
                else
                    name = QLatin1Char('-') + name;
            } else if (! symbl->asScope() || symbl->asFunction()) {
                QString type = overviewModel->m_overview.prettyType(symbl->type());
                if (Function *f = symbl->type()->asFunctionType()) {
                    name += type;
                    type = overviewModel->m_overview.prettyType(f->returnType());
                }
                if (! type.isEmpty())
                    name += QLatin1String(": ") + type;
            }
            return name;
        }

        case Qt::EditRole: {
            QString name = overviewModel->m_overview.prettyName(symbol->name());
            if (name.isEmpty())
                name = QLatin1String("anonymous");
            return name;
        }

        case Qt::DecorationRole:
            return Icons::iconForSymbol(symbol);

        case OverviewModel::FileNameRole:
            return QString::fromUtf8(symbol->fileName(), symbol->fileNameLength());

        case OverviewModel::LineNumberRole:
            return symbol->line();

        default:
            return QVariant();
        } // switch
    }

    CPlusPlus::Symbol *symbol = nullptr; // not owned
};

int OverviewModel::globalSymbolCount() const
{
    int count = 0;
    if (m_cppDocument)
        count += m_cppDocument->globalSymbolCount();
    return count;
}

Symbol *OverviewModel::globalSymbolAt(int index) const
{ return m_cppDocument->globalSymbolAt(index); }

Symbol *OverviewModel::symbolFromIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;
    auto item = static_cast<const SymbolItem*>(itemForIndex(index));
    return item ? item->symbol : nullptr;
}

OverviewModel::OverviewModel(QObject *parent)
    : Utils::TreeModel<>(parent)
{
    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(500);
    connect(m_updateTimer, &QTimer::timeout, this, &OverviewModel::rebuild);
}

Qt::ItemFlags OverviewModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

Qt::DropActions OverviewModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

QStringList OverviewModel::mimeTypes() const
{
    return Utils::DropSupport::mimeTypesForFilePaths();
}

QMimeData *OverviewModel::mimeData(const QModelIndexList &indexes) const
{
    auto mimeData = new Utils::DropMimeData;
    for (const QModelIndex &index : indexes) {
        const QVariant fileName = data(index, FileNameRole);
        if (!fileName.canConvert<QString>())
            continue;
        const QVariant lineNumber = data(index, LineNumberRole);
        if (!lineNumber.canConvert<unsigned>())
            continue;
        mimeData->addFile(Utils::FilePath::fromVariant(fileName),
                          static_cast<int>(lineNumber.value<unsigned>()));
    }
    return mimeData;
}

void OverviewModel::update(CPlusPlus::Document::Ptr doc)
{
    m_cppDocument = doc;
    if (doc)
        m_updateTimer->start();
}

int OverviewModel::editorRevision()
{
    return m_cppDocument ? m_cppDocument->editorRevision() : 0;
}

void OverviewModel::rebuild()
{
    beginResetModel();
    auto root = new SymbolItem;
    if (m_cppDocument)
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
    lineColumn.line = symbol->line();
    lineColumn.column = symbol->column();
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
        int rows = globalSymbolCount();
        for (int row = 0; row < rows; ++row) {
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

} // namespace CppEditor::Internal
