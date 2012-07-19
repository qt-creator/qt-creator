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
#ifndef GLOBAL_PARSE_STATE_H
#define GLOBAL_PARSE_STATE_H

#include <QByteArray>
#include <QStack>

namespace Debugger {
namespace Internal {
class NameDemanglerPrivate;
class ParseTreeNode;

class GlobalParseState
{
    friend class NameDemanglerPrivate;
public:
    char peek(int ahead = 0);
    char advance(int steps = 1);
    QByteArray readAhead(int charCount) const;

    int stackElementCount() const { return m_parseStack.count(); }
    ParseTreeNode *stackTop() const { return m_parseStack.top(); }
    ParseTreeNode *stackElementAt(int index) const { return m_parseStack.at(index); }
    void pushToStack(ParseTreeNode *node) { m_parseStack.push(node); }
    ParseTreeNode *popFromStack() { return m_parseStack.pop(); }

    int substitutionCount() const { return m_substitutions.count(); }
    QByteArray substitutionAt(int index) const { return m_substitutions.at(index); }
    void addSubstitution(const ParseTreeNode *node);
    void addSubstitution(const QByteArray &symbol);

    int templateParamCount() const { return m_templateParams.count(); }
    ParseTreeNode *templateParamAt(int index) const { return m_templateParams.at(index); }
    void addTemplateParam(ParseTreeNode *node) { m_templateParams << node; }
private:
    int m_pos;
    QByteArray m_mangledName;
    QList<QByteArray> m_substitutions;
    QList<ParseTreeNode *> m_templateParams;
    QStack<ParseTreeNode *> m_parseStack;

    static const char eoi = '$';
};

} // namespace Internal
} // namespace Debugger

#endif // GLOBAL_PARSE_STATE_H
