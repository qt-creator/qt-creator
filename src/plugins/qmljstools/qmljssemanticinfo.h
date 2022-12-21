// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsstaticanalysismessage.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <QTextCursor>

namespace QmlJS {
class ScopeChain;
namespace AST { class Node; }
}

namespace QmlJSTools {

class QMLJSTOOLS_EXPORT Range
{
public:
    // attributes
    QmlJS::AST::Node *ast = nullptr;
    QTextCursor begin;
    QTextCursor end;
};

class QMLJSTOOLS_EXPORT SemanticInfo
{
public:
    SemanticInfo() = default;
    explicit SemanticInfo(QmlJS::ScopeChain *rootScopeChain);

    bool isValid() const;
    int revision() const;

    // Returns the AST path
    QList<QmlJS::AST::Node *> astPath(int cursorPosition) const;

    // Returns the AST node at the offset (the last member of the astPath)
    QmlJS::AST::Node *astNodeAt(int cursorPosition) const;

    // Returns the list of declaration-type nodes that enclose the given position.
    // It is more robust than astPath because it tracks ranges with text cursors
    // and will thus be correct even if the document was changed and not yet
    // reparsed. It does not return the full path of AST nodes.
    QList<QmlJS::AST::Node *> rangePath(int cursorPosition) const;

    // Returns the declaring member
    QmlJS::AST::Node *rangeAt(int cursorPosition) const;
    QmlJS::AST::Node *declaringMemberNoProperties(int cursorPosition) const;

    // Returns a scopeChain for the given path
    QmlJS::ScopeChain scopeChain(const QList<QmlJS::AST::Node *> &path = QList<QmlJS::AST::Node *>()) const;

    void setRootScopeChain(QSharedPointer<const QmlJS::ScopeChain> rootScopeChain);

public: // attributes
    QmlJS::Document::Ptr document;
    QmlJS::Snapshot snapshot;
    QmlJS::ContextPtr context;
    QList<Range> ranges;
    QHash<QString, QList<QmlJS::SourceLocation> > idLocations;

    // these are in addition to the parser messages in the document
    QList<QmlJS::DiagnosticMessage> semanticMessages;
    QList<QmlJS::StaticAnalysis::Message> staticAnalysisMessages;

private:
    QSharedPointer<const QmlJS::ScopeChain> m_rootScopeChain;
};

} // namespace QmlJSTools

Q_DECLARE_METATYPE(QmlJSTools::SemanticInfo)
