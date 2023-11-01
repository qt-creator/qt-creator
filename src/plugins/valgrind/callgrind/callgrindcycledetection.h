// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QHash>
#include <QStack>

namespace Valgrind::Callgrind {

class Function;
class ParseData;

namespace Internal {

/**
 * Implementation of Tarjan's strongly connected components algorithm, to find function cycles,
 * as suggested by the GProf paper:
 *
 * ``gprof: A Call Graph Execution Profiler'', by S. Graham,  P.  Kessler,
 *      M.  McKusick; Proceedings of the SIGPLAN '82 Symposium on Compiler Construction,
 * SIGPLAN Notices, Vol. 17, No   6, pp. 120-126, June 1982.
 */
class CycleDetection
{
public:
    explicit CycleDetection(ParseData *data);
    QList<const Function *> run(const QList<const Function *> &input);

private:
    ParseData *m_data;

    struct Node {
        int dfs;
        int lowlink;
        const Function *function;
    };

    void tarjan(Node *node);
    void tarjanForChildNode(Node *node, Node *childNode);

    QHash<const Function *, Node *> m_nodes;
    QStack<Node *> m_stack;
    QList<const Function *> m_ret;
    int m_depth = 0;

    int m_cycle = 0;
};

} // namespace Internal

} // namespace Valgrind::Callgrind
