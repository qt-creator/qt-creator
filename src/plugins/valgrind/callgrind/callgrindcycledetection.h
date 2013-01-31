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

#ifndef LIBVALGRIND_CALLGRINDCYCLEDETECTION_H
#define LIBVALGRIND_CALLGRINDCYCLEDETECTION_H

#include <QHash>
#include <QStack>

namespace Valgrind {
namespace Callgrind {

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
    QVector<const Function *> run(const QVector<const Function *> &input);

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
    QVector<const Function *> m_ret;
    int m_depth;

    int m_cycle;
};

} // namespace Internal

} // namespace Callgrind
} // namespace Valgrind

#endif // LIBVALGRIND_CALLGRINDCYCLEDETECTION_H
