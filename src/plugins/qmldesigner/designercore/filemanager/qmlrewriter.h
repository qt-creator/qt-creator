/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLREWRITER_H
#define QMLREWRITER_H

#include "textmodifier.h"

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsutils.h>

#include <QStack>
#include <QString>

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

    QmlDesigner::TextModifier *textModifier() const
    { return m_textModifier; }

    bool includeSurroundingWhitespace(int &start, int &end) const;
    void includeLeadingEmptyLine(int &start) const;

    static QmlJS::AST::UiObjectMemberList *searchMemberToInsertAfter(QmlJS::AST::UiObjectMemberList *members, const PropertyNameList &propertyOrder);
    static QmlJS::AST::UiObjectMemberList *searchMemberToInsertAfter(QmlJS::AST::UiObjectMemberList *members, const QmlDesigner::PropertyName &propertyName, const PropertyNameList &propertyOrder);

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
