/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "clangoverviewmodel.h"

#include "clangeditordocumentprocessor.h"
#include "clangutils.h"

#include <cplusplus/Icons.h>

#include <utils/dropsupport.h>
#include <utils/linecolumn.h>
#include <utils/qtcassert.h>

using TokenContainer = ClangBackEnd::TokenInfoContainer;
using TokenContainers = QVector<TokenContainer>;

namespace ClangCodeModel {
namespace Internal {

void addFirstItem(TokenTreeItem *root)
{
    ClangBackEnd::ExtraInfo extraInfo;
    if (!root->childCount()) {
        extraInfo.token = Utf8String::fromString(
                    QString(QT_TRANSLATE_NOOP("ClangCodeModel", "<No Symbols>")));
    } else {
        extraInfo.token = Utf8String::fromString(
                    QString(QT_TRANSLATE_NOOP("ClangCodeModel", "<Select Symbol>")));
    }
    ClangBackEnd::HighlightingTypes types;
    types.mainHighlightingType = ClangBackEnd::HighlightingType::Invalid;
    TokenContainer firstItem(0, 0, 0, types, extraInfo);
    root->prependChild(new TokenTreeItem(firstItem));
}

void buildTree(const TokenContainers &containers,
               TokenTreeItem *root)
{
    // Most of the nodes are not used in this tree at all (all local variables and more)
    // therefore use unordered_map instead of vector.
    std::unordered_map<int, TokenTreeItem *> treeItemCache;
    for (int index = 0; index < containers.size(); ++index) {
        const TokenContainer &container = containers[index];
        if (!container.isGlobalDeclaration())
            continue;

        const int lexicalParentIndex = container.extraInfo.lexicalParentIndex;
        QTC_ASSERT(lexicalParentIndex < index, return;);

        auto item = std::make_unique<TokenTreeItem>(container);
        treeItemCache[index] = item.get();

        TokenTreeItem *parent = root;
        if (lexicalParentIndex >= 0 && treeItemCache[lexicalParentIndex])
            parent = treeItemCache[lexicalParentIndex];

        ClangBackEnd::HighlightingType parentType = parent->token.types.mainHighlightingType;
        if (parentType == ClangBackEnd::HighlightingType::VirtualFunction
                || parentType == ClangBackEnd::HighlightingType::Function) {
            // Treat everything inside a function scope as local variables.
            treeItemCache.erase(index);
            continue;
        }

        parent->appendChild(item.release());
    }

    addFirstItem(root);
}

static QString addType(const QString &name, const ClangBackEnd::ExtraInfo &extraInfo)
{
    return name + QLatin1String(" -> ", 4) + extraInfo.typeSpelling.toString();
}

static QString fullName(const ClangBackEnd::ExtraInfo &extraInfo, TokenTreeItem *parent)
{
    const QString parentType = parent->token.extraInfo.typeSpelling.toString();
    if (extraInfo.semanticParentTypeSpelling.startsWith(parentType)) {
        const QString parentQualification = parentType.isEmpty()
                ? extraInfo.semanticParentTypeSpelling
                : extraInfo.semanticParentTypeSpelling.mid(parentType.length() + 2);
        if (!parentQualification.isEmpty())
            return parentQualification + "::" + extraInfo.token.toString();
    }

    return extraInfo.token.toString();
}

QVariant TokenTreeItem::data(int column, int role) const
{
    Q_UNUSED(column)

    if (token.types.mainHighlightingType == ClangBackEnd::HighlightingType::Invalid
            && token.line == 0 && token.column == 0 && token.length == 0) {
        if (role == Qt::DisplayRole)
            return token.extraInfo.token.toString();
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole: {
        QString name = fullName(token.extraInfo, static_cast<TokenTreeItem *>(parent()));

        ClangBackEnd::HighlightingType mainType = token.types.mainHighlightingType;

        if (mainType == ClangBackEnd::HighlightingType::VirtualFunction
                    || mainType == ClangBackEnd::HighlightingType::Function) {
            name = addType(name, token.extraInfo);
        } else if (mainType == ClangBackEnd::HighlightingType::GlobalVariable
                   || mainType == ClangBackEnd::HighlightingType::Field
                   || mainType == ClangBackEnd::HighlightingType::QtProperty) {
            name = addType(name, token.extraInfo);
            if (token.types.mixinHighlightingTypes.contains(
                                        ClangBackEnd::HighlightingType::ObjectiveCProperty)) {
                name = QLatin1String("@property ") + name;
            } else if (token.types.mixinHighlightingTypes.contains(
                                               ClangBackEnd::HighlightingType::ObjectiveCMethod)) {
                if (token.extraInfo.storageClass == ClangBackEnd::StorageClass::Static)
                    name = QLatin1Char('+') + name;
                else
                    name = QLatin1Char('-') + name;
            }
        } else if (mainType == ClangBackEnd::HighlightingType::Type) {

            if (token.types.mixinHighlightingTypes.contains(
                        ClangBackEnd::HighlightingType::ObjectiveCClass)) {
                name = QLatin1String("@class ") + name;
            } else if (token.types.mixinHighlightingTypes.contains(
                                               ClangBackEnd::HighlightingType::ObjectiveCProtocol)) {
                name = QLatin1String("@protocol ") + name;
            } else if (token.types.mixinHighlightingTypes.contains(
                                               ClangBackEnd::HighlightingType::ObjectiveCInterface)) {
                name = QLatin1String("@interface ") + name;
            } else if (token.types.mixinHighlightingTypes.contains(
                           ClangBackEnd::HighlightingType::ObjectiveCImplementation)) {
                name = QLatin1String("@implementation ") + name;
            } else if (token.types.mixinHighlightingTypes.contains(
                                               ClangBackEnd::HighlightingType::ObjectiveCCategory)) {
                name = name + " [category]";
            }
        }
        return name;
    }

    case Qt::EditRole: {
        return token.extraInfo.token.toString();
    }

    case Qt::DecorationRole: {
        return ::Utils::CodeModelIcon::iconForType(ClangCodeModel::Utils::iconTypeForToken(token));
    }

    case CppTools::AbstractOverviewModel::FileNameRole: {
        return token.extraInfo.cursorRange.start.filePath.toString();
    }

    case CppTools::AbstractOverviewModel::LineNumberRole: {
        return token.line;
    }

    default:
        return QVariant();
    } // switch
}

bool OverviewModel::rebuild(const QString &filePath)
{
    ClangEditorDocumentProcessor *processor = ClangEditorDocumentProcessor::get(filePath);
    if (!processor)
        return false;
    if (m_filePath != filePath) {
        if (!m_filePath.isEmpty()) {
            ClangEditorDocumentProcessor *previousProcessor
                    = ClangEditorDocumentProcessor::get(m_filePath);
            if (previousProcessor) {
                disconnect(previousProcessor, &ClangEditorDocumentProcessor::tokenInfosUpdated,
                           this, &OverviewModel::needsUpdate);
            }
        }
        m_filePath = filePath;
        connect(processor, &ClangEditorDocumentProcessor::tokenInfosUpdated, this,
                &OverviewModel::needsUpdate);
    }

    const TokenContainers &tokenContainers = processor->tokenInfos();
    auto *root = new TokenTreeItem;
    buildTree(tokenContainers, root);
    setRootItem(root);

    return true;
}

bool OverviewModel::isGenerated(const QModelIndex &) const
{
    return false;
}

::Utils::Link OverviewModel::linkFromIndex(const QModelIndex &sourceIndex) const
{
    auto item = static_cast<TokenTreeItem *>(itemForIndex(sourceIndex));
    if (!item)
        return {};
    return ::Utils::Link(m_filePath, static_cast<int>(item->token.line),
                         static_cast<int>(item->token.column) - 1);
}

::Utils::LineColumn OverviewModel::lineColumnFromIndex(const QModelIndex &sourceIndex) const
{
    auto item = static_cast<TokenTreeItem *>(itemForIndex(sourceIndex));
    if (!item)
        return {};
    return {static_cast<int>(item->token.line),
            static_cast<int>(item->token.column)};
}

OverviewModel::Range OverviewModel::rangeFromIndex(const QModelIndex &sourceIndex) const
{
    auto item = static_cast<TokenTreeItem *>(itemForIndex(sourceIndex));
    if (!item)
        return {};
    const ClangBackEnd::SourceRangeContainer &range = item->token.extraInfo.cursorRange;
    return std::make_pair(::Utils::LineColumn(static_cast<int>(range.start.line),
                                              static_cast<int>(range.start.column)),
                          ::Utils::LineColumn(static_cast<int>(range.end.line),
                                              static_cast<int>(range.end.column)));
}

} // namespace Internal
} // namespace ClangCodeModel
