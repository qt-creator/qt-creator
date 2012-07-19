/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QMLJSSEMANTICINFO_H
#define QMLJSSEMANTICINFO_H

#include "qmljstools_global.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsstaticanalysismessage.h>

#include <QTextCursor>

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

#endif // QMLJSSEMANTICINFO_H
