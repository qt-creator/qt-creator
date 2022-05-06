/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#pragma once

#include <coreplugin/helpitem.h>
#include <languageserverprotocol/jsonobject.h>

#include <functional>

namespace LanguageClient { class Client; }
namespace LanguageServerProtocol {
class MessageId;
class Position;
class Range;
}
namespace Utils { class FilePath; }

QT_BEGIN_NAMESPACE
class QStringView;
QT_END_NAMESPACE

namespace ClangCodeModel::Internal {

// string view helpers
QStringView subViewLen(const QString &s, qsizetype start, qsizetype length);
QStringView subViewEnd(const QString &s, qsizetype start, qsizetype end);

class ClangdAstNode : public LanguageServerProtocol::JsonObject
{
public:
    using JsonObject::JsonObject;

    // The general kind of node, such as “expression”. Corresponds to clang’s base AST node type,
    // such as Expr. The most common are “expression”, “statement”, “type” and “declaration”.
    QString role() const;

    // The specific kind of node, such as “BinaryOperator”. Corresponds to clang’s concrete
    // node class, with Expr etc suffix dropped.
    QString kind() const;

    // Brief additional details, such as ‘||’. Information present here depends on the node kind.
    Utils::optional<QString> detail() const;

    // One line dump of information, similar to that printed by clang -Xclang -ast-dump.
    // Only available for certain types of nodes.
    Utils::optional<QString> arcana() const;

    // The part of the code that produced this node. Missing for implicit nodes, nodes produced
    // by macro expansion, etc.
    LanguageServerProtocol::Range range() const;

    // Descendants describing the internal structure. The tree of nodes is similar to that printed
    // by clang -Xclang -ast-dump, or that traversed by clang::RecursiveASTVisitor.
    Utils::optional<QList<ClangdAstNode>> children() const;

    bool hasRange() const;
    bool arcanaContains(const QString &s) const;
    bool detailIs(const QString &s) const { return detail() && *detail() == s; }
    bool isFunction() const;
    bool isMemberFunctionCall() const;
    bool isPureVirtualDeclaration() const;
    bool isPureVirtualDefinition() const;
    bool mightBeAmbiguousVirtualCall() const;
    bool isNamespace() const { return role() == "declaration" && kind() == "Namespace"; }
    bool isTemplateParameterDeclaration() const;;
    QString type() const;
    QString typeFromPos(const QString &s, int pos) const;
    Core::HelpItem::Category qdocCategoryForDeclaration(Core::HelpItem::Category fallback);

    // Returns true <=> the type is "recursively const".
    // E.g. returns true for "const int &", "const int *" and "const int * const *",
    // and false for "int &" and "const int **".
    // For non-pointer types such as "int", we check whether they are used as lvalues
    // or rvalues.
    bool hasConstType() const;

    bool childContainsRange(int index, const LanguageServerProtocol::Range &range) const;
    bool hasChildWithRole(const QString &role) const;
    QString operatorString() const;

    enum class FileStatus { Ours, Foreign, Mixed, Unknown };
    FileStatus fileStatus(const Utils::FilePath &thisFile) const;

    // For debugging.
    void print(int indent = 0) const;

    bool isValid() const override;
};
using ClangdAstPath = QList<ClangdAstNode>;

ClangdAstPath getAstPath(const ClangdAstNode &root, const LanguageServerProtocol::Range &range);
ClangdAstPath getAstPath(const ClangdAstNode &root, const LanguageServerProtocol::Position &pos);

using AstHandler = std::function<void(const ClangdAstNode &node,
                                      const LanguageServerProtocol::MessageId &requestId)>;
LanguageServerProtocol::MessageId requestAst(LanguageClient::Client *client,
        const Utils::FilePath &filePath, const LanguageServerProtocol::Range range,
        const AstHandler &handler);

} // namespace ClangCodeModel::Internal

