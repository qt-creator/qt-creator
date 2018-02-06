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

static bool contains(const ClangBackEnd::SourceRangeContainer &range,
                     unsigned line,
                     unsigned column)
{
    if (line < range.start().line() || line > range.end().line())
        return false;
    if (line == range.start().line() && column < range.start().column())
        return false;
    if (line == range.end().line() && column > range.end().column())
        return false;
    return true;
}

static bool contains(const ClangBackEnd::SourceRangeContainer &range,
                     const ClangBackEnd::SourceLocationContainer &location)
{
    return contains(range, location.line(), location.column());
}

void buildTree(TokenContainers::const_iterator containersBegin,
               TokenContainers::const_iterator containersEnd,
               TokenTreeItem *parent, bool isRoot = false)
{
    for (auto it = containersBegin; it != containersEnd;) {
        if (!it->extraInfo().declaration) {
            ++it;
            continue;
        }
        if (it->types().mainHighlightingType == ClangBackEnd::HighlightingType::LocalVariable) {
            ++it;
            continue;
        }

        auto *item = new TokenTreeItem(*it);
        parent->appendChild(item);
        const auto &range = it->extraInfo().cursorRange;

        ++it;
        auto innerIt = it;
        for (; innerIt != containersEnd; ++innerIt) {
            if (!innerIt->extraInfo().declaration)
                continue;
            if (innerIt->types().mainHighlightingType
                    == ClangBackEnd::HighlightingType::LocalVariable) {
                continue;
            }
            const auto &start = innerIt->extraInfo().cursorRange.start();
            if (!contains(range, start)) {
                break;
            }
        }
        if (innerIt != it) {
            buildTree(it, innerIt, item);
            it = innerIt;
        }
    }

    if (isRoot) {
        ClangBackEnd::ExtraInfo extraInfo;
        if (!parent->childCount()) {
            extraInfo.token = Utf8String::fromString(
                        QString(QT_TRANSLATE_NOOP("ClangCodeModel", "<No Symbols>")));
        } else {
            extraInfo.token = Utf8String::fromString(
                        QString(QT_TRANSLATE_NOOP("ClangCodeModel", "<Select Symbol>")));
        }
        ClangBackEnd::HighlightingTypes types;
        types.mainHighlightingType = ClangBackEnd::HighlightingType::Invalid;
        TokenContainer firstItem(0, 0, 0, types, extraInfo);
        parent->prependChild(new TokenTreeItem(firstItem));
    }
}

static QString addResultTypeToFunctionSignature(const QString &signature,
                                                const ClangBackEnd::ExtraInfo &extraInfo)
{
    return signature + extraInfo.typeSpelling.toString() + QLatin1String(" -> ", 4)
            + extraInfo.resultTypeSpelling.toString();
}

static QString addTypeToVariableName(const QString &name, const ClangBackEnd::ExtraInfo &extraInfo)
{
    return name + QLatin1String(" -> ", 4) + extraInfo.typeSpelling.toString();
}

QVariant TokenTreeItem::data(int column, int role) const
{
    Q_UNUSED(column)

    if (token.types().mainHighlightingType == ClangBackEnd::HighlightingType::Invalid
            && token.line() == 0 && token.column() == 0 && token.length() == 0) {
        if (role == Qt::DisplayRole)
            return token.extraInfo().token.toString();
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole: {
        QString name = token.extraInfo().token.toString();
        ClangBackEnd::HighlightingType mainType = token.types().mainHighlightingType;

        if (mainType == ClangBackEnd::HighlightingType::VirtualFunction
                    || mainType == ClangBackEnd::HighlightingType::Function) {
            name = addResultTypeToFunctionSignature(name, token.extraInfo());
        } else if (mainType == ClangBackEnd::HighlightingType::GlobalVariable
                   || mainType == ClangBackEnd::HighlightingType::Field
                   || mainType == ClangBackEnd::HighlightingType::QtProperty) {
            name = addTypeToVariableName(name, token.extraInfo());
            if (token.types().mixinHighlightingTypes.contains(
                                        ClangBackEnd::HighlightingType::ObjectiveCProperty)) {
                name = QLatin1String("@property ") + name;
            } else if (token.types().mixinHighlightingTypes.contains(
                                               ClangBackEnd::HighlightingType::ObjectiveCMethod)) {
                if (token.extraInfo().storageClass == ClangBackEnd::StorageClass::Static)
                    name = QLatin1Char('+') + name;
                else
                    name = QLatin1Char('-') + name;
            }
        } else if (mainType == ClangBackEnd::HighlightingType::Type) {

            if (token.types().mixinHighlightingTypes.contains(
                        ClangBackEnd::HighlightingType::ObjectiveCClass)) {
                name = QLatin1String("@class ") + name;
            } else if (token.types().mixinHighlightingTypes.contains(
                                               ClangBackEnd::HighlightingType::ObjectiveCProtocol)) {
                name = QLatin1String("@protocol ") + name;
            } else if (token.types().mixinHighlightingTypes.contains(
                                               ClangBackEnd::HighlightingType::ObjectiveCInterface)) {
                name = QLatin1String("@interface ") + name;
            } else if (token.types().mixinHighlightingTypes.contains(
                           ClangBackEnd::HighlightingType::ObjectiveCImplementation)) {
                name = QLatin1String("@implementation ") + name;
            } else if (token.types().mixinHighlightingTypes.contains(
                                               ClangBackEnd::HighlightingType::ObjectiveCCategory)) {
                name = name + " [category]";
            }
        }
        return name;
    }

    case Qt::EditRole: {
        return token.extraInfo().token.toString();
    }

    case Qt::DecorationRole: {
        return CPlusPlus::Icons::iconForType(ClangCodeModel::Utils::iconTypeForToken(token));
    }

    case CppTools::AbstractOverviewModel::FileNameRole: {
        return token.extraInfo().cursorRange.start().filePath().toString();
    }

    case CppTools::AbstractOverviewModel::LineNumberRole: {
        return token.line();
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
    m_filePath = filePath;

    const TokenContainers &tokenContainers = processor->tokenInfos();
    auto *root = new TokenTreeItem;
    buildTree(tokenContainers.begin(), tokenContainers.end(), root, true);
    setRootItem(root);

    return true;
}

bool OverviewModel::isGenerated(const QModelIndex &) const
{
    return false;
}

::Utils::Link OverviewModel::linkFromIndex(const QModelIndex &sourceIndex) const
{
    TokenTreeItem *item = static_cast<TokenTreeItem *>(itemForIndex(sourceIndex));
    if (!item)
        return {};
    return ::Utils::Link(m_filePath, static_cast<int>(item->token.line()),
                         static_cast<int>(item->token.column()) - 1);
}

::Utils::LineColumn OverviewModel::lineColumnFromIndex(const QModelIndex &sourceIndex) const
{
    TokenTreeItem *item = static_cast<TokenTreeItem *>(itemForIndex(sourceIndex));
    if (!item)
        return {};
    ::Utils::LineColumn lineColumn;
    lineColumn.line = static_cast<int>(item->token.line());
    lineColumn.column = static_cast<int>(item->token.column());
    return lineColumn;
}

} // namespace Internal
} // namespace ClangCodeModel
