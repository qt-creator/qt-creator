/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLREWRITER_H
#define QMLREWRITER_H

#include "textmodifier.h"

#include <qmljs/parser/qmljsastvisitor_p.h>

#include <QtCore/QStack>
#include <QtCore/QString>

namespace QmlDesigner {
namespace Internal {

class QMLRewriter: protected QmlJS::AST::Visitor
{
public:
    typedef QStack<QmlJS::AST::Node *> ASTPath;

public:
    QMLRewriter(QmlDesigner::TextModifier &textModifier);

    virtual bool operator()(QmlJS::AST::UiProgram *ast);

    static void dump(const ASTPath &path);

protected:
    using QmlJS::AST::Visitor::visit;

    virtual void replace(int offset, int length, const QString &text);
    virtual void move(const QmlDesigner::TextModifier::MoveInfo &moveInfo);

    QString textBetween(int startPosition, int endPosition) const;
    QString textAt(const QmlJS::AST::SourceLocation &location) const;

    int indentDepth() const
    { return textModifier()->indentDepth(); }
    unsigned calculateIndentDepth(const QmlJS::AST::SourceLocation &position) const;
    static QString addIndentation(const QString &text, unsigned depth);
    static QString removeIndentation(const QString &text, unsigned depth);
    static QString removeIndentationFromLine(const QString &text, int depth);

    static QmlJS::AST::SourceLocation calculateLocation(QmlJS::AST::UiQualifiedId *id);
    static bool isMissingSemicolon(QmlJS::AST::UiObjectMember *member);
    static bool isMissingSemicolon(QmlJS::AST::Statement *stmt);
    static QString flatten(QmlJS::AST::UiQualifiedId *first);

    QmlDesigner::TextModifier *textModifier() const
    { return m_textModifier; }

    bool includeSurroundingWhitespace(int &start, int &end) const;
    void includeLeadingEmptyLine(int &start) const;

    static QmlJS::AST::UiObjectMemberList *searchMemberToInsertAfter(QmlJS::AST::UiObjectMemberList *members, const QStringList &propertyOrder);
    static QmlJS::AST::UiObjectMemberList *searchMemberToInsertAfter(QmlJS::AST::UiObjectMemberList *members, const QString &propertyName, const QStringList &propertyOrder);

protected:
    bool didRewriting() const
    { return m_didRewriting; }

    void setDidRewriting(bool didRewriting)
    { m_didRewriting = didRewriting; }

private:
    QmlDesigner::TextModifier *m_textModifier;
    bool m_didRewriting;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // QMLREWRITER_H
