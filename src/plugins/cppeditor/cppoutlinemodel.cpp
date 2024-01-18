// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppoutlinemodel.h"

#include <cplusplus/Icons.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Symbols.h>

#include <utils/link.h>
#include <utils/theme/theme.h>

#include <QTimer>

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
                    return QString(QT_TRANSLATE_NOOP("QtC::CppEditor", "<Select Symbol>"));
                return QString(QT_TRANSLATE_NOOP("QtC::CppEditor", "<No Symbols>"));
            default:
                return QVariant();
            }
        }

        auto outlineModel = qobject_cast<const OutlineModel*>(model());
        if (!symbol || !outlineModel)
            return QVariant();

        switch (role) {
        case Qt::DisplayRole: {
            QString name = outlineModel->m_overview.prettyName(symbol->name());
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
                    name += QString(" (%1)").arg(
                        outlineModel->m_overview.prettyName(clazz->categoryName()));
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
                        parameters.append(outlineModel->m_overview.prettyName(
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
                QString type = outlineModel->m_overview.prettyType(symbl->type());
                if (Function *f = symbl->type()->asFunctionType()) {
                    name += type;
                    type = outlineModel->m_overview.prettyType(f->returnType());
                }
                if (! type.isEmpty())
                    name += QLatin1String(": ") + type;
            }
            return name;
        }

        case Qt::EditRole: {
            QString name = outlineModel->m_overview.prettyName(symbol->name());
            if (name.isEmpty())
                name = QLatin1String("anonymous");
            return name;
        }

        case Qt::ForegroundRole: {
            const auto isFwdDecl = [&] {
                const FullySpecifiedType type = symbol->type();
                if (type->asForwardClassDeclarationType())
                    return true;
                if (const Template * const tmpl = type->asTemplateType())
                    return tmpl->declaration() && tmpl->declaration()->asForwardClassDeclaration();
                if (type->asObjCForwardClassDeclarationType())
                    return true;
                if (type->asObjCForwardProtocolDeclarationType())
                    return true;
                return false;
            };
            if (isFwdDecl())
                return Utils::creatorTheme()->color(Utils::Theme::TextColorDisabled);
            return TreeItem::data(column, role);
        }

        case Qt::DecorationRole:
            return Icons::iconForSymbol(symbol);

        case OutlineModel::FileNameRole:
            return QString::fromUtf8(symbol->fileName(), symbol->fileNameLength());

        case OutlineModel::LineNumberRole:
            return symbol->line();

        default:
            return QVariant();
        } // switch
    }

    CPlusPlus::Symbol *symbol = nullptr; // not owned
};

int OutlineModel::globalSymbolCount() const
{
    int count = 0;
    if (m_cppDocument)
        count += m_cppDocument->globalSymbolCount();
    return count;
}

Symbol *OutlineModel::globalSymbolAt(int index) const
{ return m_cppDocument->globalSymbolAt(index); }

Symbol *OutlineModel::symbolFromIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;
    auto item = static_cast<const SymbolItem*>(itemForIndex(index));
    return item ? item->symbol : nullptr;
}

OutlineModel::OutlineModel(QObject *parent)
    : Utils::TreeModel<>(parent)
{
    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(500);
    connect(m_updateTimer, &QTimer::timeout, this, &OutlineModel::rebuild);
}

Qt::ItemFlags OutlineModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

Qt::DropActions OutlineModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

QStringList OutlineModel::mimeTypes() const
{
    return Utils::DropSupport::mimeTypesForFilePaths();
}

QMimeData *OutlineModel::mimeData(const QModelIndexList &indexes) const
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

void OutlineModel::update(CPlusPlus::Document::Ptr doc)
{
    m_candidate = doc;
    m_updateTimer->start();
}

int OutlineModel::editorRevision()
{
    return m_cppDocument ? m_cppDocument->editorRevision() : -1;
}

void OutlineModel::rebuild()
{
    beginResetModel();
    m_cppDocument = m_candidate;
    m_candidate.reset();
    auto root = new SymbolItem;
    if (m_cppDocument)
        buildTree(root, true);
    setRootItemInternal(root);
    endResetModel();
}

bool OutlineModel::isGenerated(const QModelIndex &sourceIndex) const
{
    CPlusPlus::Symbol *symbol = symbolFromIndex(sourceIndex);
    return symbol && symbol->isGenerated();
}

Utils::Link OutlineModel::linkFromIndex(const QModelIndex &sourceIndex) const
{
    CPlusPlus::Symbol *symbol = symbolFromIndex(sourceIndex);
    if (!symbol)
        return {};

    return symbol->toLink();
}

Utils::Text::Position OutlineModel::positionFromIndex(const QModelIndex &sourceIndex) const
{
    Utils::Text::Position lineColumn;
    CPlusPlus::Symbol *symbol = symbolFromIndex(sourceIndex);
    if (!symbol)
        return lineColumn;
    lineColumn.line = symbol->line();
    lineColumn.column = symbol->column() - 1;
    return lineColumn;
}

OutlineModel::Range OutlineModel::rangeFromIndex(const QModelIndex &sourceIndex) const
{
    Utils::Text::Position lineColumn = positionFromIndex(sourceIndex);
    return {lineColumn, lineColumn};
}

void OutlineModel::buildTree(SymbolItem *root, bool isRoot)
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

static bool contains(const OutlineModel::Range &range, int line, int column)
{
    if (line < range.first.line || line > range.second.line)
        return false;
    if (line == range.first.line && column < range.first.column)
        return false;
    if (line == range.second.line && column > range.second.column)
        return false;
    return true;
}

QModelIndex OutlineModel::indexForPosition(int line, int column,
                                           const QModelIndex &rootIndex) const
{
    QModelIndex lastIndex = rootIndex;
    const int rowCount = this->rowCount(rootIndex);
    for (int row = 0; row < rowCount; ++row) {
        const QModelIndex index = this->index(row, 0, rootIndex);
        const OutlineModel::Range range = rangeFromIndex(index);
        if (range.first.line > line)
            break;
        // Skip ranges that do not include current line and column.
        if (range.second != range.first && !contains(range, line, column))
            continue;
        lastIndex = index;
    }

    if (lastIndex != rootIndex) {
        // recurse
        lastIndex = indexForPosition(line, column, lastIndex);
    }

    return lastIndex;
}

} // namespace CppEditor::Internal
