// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindcycledetection.h"

#include "callgrindfunction.h"
#include "callgrindfunctioncall.h"
#include "callgrindfunctioncycle.h"
#include "callgrindparsedata.h"

#include <utils/qtcassert.h>

namespace Valgrind::Callgrind::Internal {

CycleDetection::CycleDetection(ParseData *data)
    : m_data(data)
{
}

QList<const Function *> CycleDetection::run(const QList<const Function *> &input)
{
    for (const Function *function : input) {
        Node *node = new Node;
        node->function = function;
        node->dfs = -1;
        node->lowlink = -1;
        m_nodes.insert(function, node);
    }
    for (Node *node : std::as_const(m_nodes)) {
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

    const QList<const FunctionCall *> calls = node->function->outgoingCalls();
    for (const FunctionCall *call : calls)
        tarjanForChildNode(node, m_nodes.value(call->callee()));

    if (node->dfs == node->lowlink) {
        QList<const Function *> functions;
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
            auto cycle = new FunctionCycle(m_data);
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

} // namespace Valgrind::Callgrind::Internal
