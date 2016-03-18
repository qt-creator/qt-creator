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
    Range(): ast(0) {}

public: // attributes
    QmlJS::AST::Node *ast;
    QTextCursor begin;
    QTextCursor end;
};

class QMLJSTOOLS_EXPORT SemanticInfo
{
public:
    SemanticInfo() {}
    SemanticInfo(QmlJS::ScopeChain *rootScopeChain);

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
    QHash<QString, QList<QmlJS::AST::SourceLocation> > idLocations;

    // these are in addition to the parser messages in the document
    QList<QmlJS::DiagnosticMessage> semanticMessages;
    QList<QmlJS::StaticAnalysis::Message> staticAnalysisMessages;

private:
    QSharedPointer<const QmlJS::ScopeChain> m_rootScopeChain;
};

} // namespace QmlJSTools

Q_DECLARE_METATYPE(QmlJSTools::SemanticInfo)
