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

#include "callgrindcycledetection.h"

#include "callgrindfunction.h"
#include "callgrindfunctioncall.h"
#include "callgrindparsedata.h"
#include "callgrindfunctioncycle.h"

#include <utils/qtcassert.h>

#include <QDebug>

namespace Valgrind {
namespace Callgrind {
namespace Internal {

CycleDetection::CycleDetection(ParseData *data)
    : m_data(data)
    , m_depth(0)
    , m_cycle(0)
{
}

QVector<const Function *> CycleDetection::run(const QVector<const Function *> &input)
{
    foreach (const Function *function, input) {
        Node *node = new Node;
        node->function = function;
        node->dfs = -1;
        node->lowlink = -1;
        m_nodes.insert(function, node);
    }
    foreach (Node *node, m_nodes) {
        if (node->dfs == -1)
            tarjan(node);
    }
    qDeleteAll(m_nodes);
    return m_ret;
}

void CycleDetection::tarjan(Node *node)
{
    QTC_ASSERT(node->dfs == -1, return);
    node->dfs = m_depth;
    node->lowlink = m_depth;

    m_depth++;
    m_stack.push(node);

    foreach (const FunctionCall *call, node->function->outgoingCalls())
        tarjanForChildNode(node, m_nodes.value(call->callee()));

    if (node->dfs == node->lowlink) {
        QVector<const Function *> functions;
        Node *n;
        do {
            n = m_stack.pop();
            functions << n->function;
        } while (n != node);

        if (functions.size() == 1) {
            // not a real cycle
            m_ret.append(node->function);
        } else {
            // actual cycle
            FunctionCycle *cycle = new FunctionCycle(m_data);
            cycle->setFile(node->function->fileId());
            m_cycle++;
            qint64 id = -1;
            m_data->addCompressedFunction(QString::fromLatin1("cycle %1").arg(m_cycle), id);
            cycle->setName(id);
            cycle->setObject(node->function->objectId());
            cycle->setFunctions(functions);
            m_ret.append(cycle);
        }
    }
}

void CycleDetection::tarjanForChildNode(Node *node, Node *childNode)
{
    if (childNode->dfs == -1) {
        tarjan(childNode);
        if (childNode->lowlink < node->lowlink)
            node->lowlink = childNode->lowlink;
    } else if (childNode->dfs < node->lowlink && m_stack.contains(childNode)) {
        node->lowlink = childNode->dfs;
    }
}

} // namespace Internal
} // namespace Callgrind
} // namespace Valgrind
