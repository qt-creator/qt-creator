/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef GLOBAL_PARSE_STATE_H
#define GLOBAL_PARSE_STATE_H

#include <QByteArray>
#include <QSharedPointer>
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
    QSharedPointer<ParseTreeNode> stackTop() const { return m_parseStack.top(); }
    QSharedPointer<ParseTreeNode> stackElementAt(int index) const { return m_parseStack.at(index); }
    void pushToStack(const QSharedPointer<ParseTreeNode> &node) { m_parseStack.push(node); }
    QSharedPointer<ParseTreeNode> popFromStack() { return m_parseStack.pop(); }

    int substitutionCount() const { return m_substitutions.count(); }
    QSharedPointer<ParseTreeNode> substitutionAt(int index) const { return m_substitutions.at(index); }
    void addSubstitution(const QSharedPointer<ParseTreeNode> &node);

    int templateParamCount() const { return m_templateParams.count(); }
    QSharedPointer<ParseTreeNode> templateParamAt(int index) const { return m_templateParams.at(index); }
    void addTemplateParam(const QSharedPointer<ParseTreeNode> &node) { m_templateParams << node; }
private:
    int m_pos;
    QByteArray m_mangledName;
    QList<QSharedPointer<ParseTreeNode> > m_substitutions;
    QList<QSharedPointer<ParseTreeNode> > m_templateParams;
    QStack<QSharedPointer<ParseTreeNode> > m_parseStack;

    static const char eoi = '$';
};

} // namespace Internal
} // namespace Debugger

#endif // GLOBAL_PARSE_STATE_H
