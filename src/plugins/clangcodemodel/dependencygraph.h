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

#ifndef DEPENDENCYGRAPH_H
#define DEPENDENCYGRAPH_H

#include "includetracker.h"

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QLinkedList>
#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtCore/QQueue>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtCore/QDebug>

namespace ClangCodeModel {
namespace Internal {

class DependencyGraph : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(DependencyGraph)

public:
    DependencyGraph();
    ~DependencyGraph();

    void addFile(const QString &fileName, const QStringList &compilationOptions);

    QFuture<void> compute();

    enum DependencyRole
    {
        FilesDirectlyIncludedBy,      // Only direct inclusions
        FilesIncludedBy,              // Both direct and indirect inclusions
        FilesWhichDirectlyInclude,    // This one is directly included from...
        FilesWhichInclude             // This one is directly or indirectly included from...
    };

    /*
     * You should use this version if you simply want all the dependencies, no matter what.
     */
    QStringList collectDependencies(const QString &referenceFile, DependencyRole role) const;

    /*
     * You should use this version if you might be interested on a particular dependency
     * and don't want to continue the search once you have found it. In this case you need
     * supply a visitor. Currently the visitor concept simply requires that a type Visitor_T
     * models a function that will receive a file string s and indicate whether or not to
     * continue:
     *
     * Visitor_T().acceptFile(s) must be a valid expression.
     *
     */
    template <class Visitor_T>
    void collectDependencies(const QString &referenceFile,
                             DependencyRole role,
                             Visitor_T *visitor) const;

    bool hasDependency(const QString &referenceFile, DependencyRole role) const;

    void discard();

signals:
    void dependencyGraphAvailable();

private:
    QList<QPair<QString, QStringList> > m_files;
    IncludeTracker m_includeTracker;
    QFutureWatcher<void> m_computeWatcher;

    void cancel();
    void computeCore();

    // The dependency graph is represent as an adjacency list. The vertices contains
    // a list of *out* edges and a list of *in* edges. Each vertex corresponds to a file.
    // Its out edges correspond to the files which get directly included by this one, while
    // its in edges correspond to files that directly include this one.
    //
    // For better space efficiency, the adjacency nodes doen't explicitly store the file
    // names themselves, but rather an iterator to the corresponding vertex. In addition,
    // for speed efficiency we keep track of a hash table that contains iterators to the
    // actual vertex storage container, which actually contains the strings for the file
    // names. The vertex container itself is a linked list, it has the semantics we need,
    // in particular regarding iterator invalidation.

    struct AdjacencyNode;
    struct Node
    {
        Node(const QString &fileName);

        QString m_fileName;
        AdjacencyNode *m_out;
        AdjacencyNode *m_in;
    };

    typedef QLinkedList<Node> NodeList;
    typedef NodeList::iterator NodeListIt;
    typedef QHash<QString, NodeListIt> NodeRefSet;
    typedef NodeRefSet::iterator NodeRefSetIt;

    struct AdjacencyNode
    {
        AdjacencyNode(NodeListIt it);

        AdjacencyNode *m_next;
        NodeListIt m_nodeIt;
    };


    void processIncludes(NodeRefSetIt currentFileIt,
                         const QStringList &compilationOptions);

    template <class Visitor_T>
    void collectFilesBFS(NodeListIt nodeIt, DependencyRole role, Visitor_T *visitor) const;


    // Core graph operations and data

    QPair<bool, NodeRefSetIt> findVertex(const QString &s) const;
    NodeRefSetIt insertVertex(const QString &s);
    void insertEdge(NodeRefSetIt fromIt, NodeRefSetIt toIt);

    void deleteAdjacencies(AdjacencyNode *node);
    void createAdjacency(AdjacencyNode **node, AdjacencyNode *newNode);

    NodeList m_nodes;
    NodeRefSet m_nodesRefs;
};

template <class Visitor_T>
void DependencyGraph::collectDependencies(const QString &referenceFile,
                                          DependencyRole role,
                                          Visitor_T *visitor) const
{
    if (m_computeWatcher.isRunning())
        return;

    QPair<bool, NodeRefSetIt> v = findVertex(referenceFile);
    if (!v.first)
        return;

    NodeListIt nodeIt = v.second.value();

    if (role == FilesDirectlyIncludedBy || role == FilesWhichDirectlyInclude) {
        AdjacencyNode *adj;
        if (role == FilesDirectlyIncludedBy)
            adj = nodeIt->m_out;
        else
            adj = nodeIt->m_in;

        for (; adj; adj = adj->m_next) {
            NodeListIt dependentIt = adj->m_nodeIt;
            if (visitor->acceptFile(dependentIt->m_fileName))
                return;
        }
    } else {
        collectFilesBFS(nodeIt, role, visitor);
    }
}

template <class Visitor_T>
void DependencyGraph::collectFilesBFS(NodeListIt nodeIt,
                                      DependencyRole role,
                                      Visitor_T *visitor) const
{
    Q_ASSERT(role == FilesIncludedBy || role == FilesWhichInclude);

    if (m_computeWatcher.isRunning())
        return;

    QQueue<NodeListIt> q;
    q.enqueue(nodeIt);

    QSet<QString> visited;
    visited.insert(nodeIt->m_fileName);

    while (!q.isEmpty()) {
        NodeListIt currentIt = q.dequeue();
        AdjacencyNode *adj;
        if (role == FilesIncludedBy)
            adj = currentIt->m_out;
        else
            adj = currentIt->m_in;
        while (adj) {
            NodeListIt adjNodeIt = adj->m_nodeIt;
            adj = adj->m_next;

            const QString &adjFileName = adjNodeIt->m_fileName;
            if (visited.contains(adjFileName))
                continue;

            if (visitor->acceptFile(adjFileName))
                return;

            visited.insert(adjFileName);
            q.enqueue(adjNodeIt);
        }
    }
}


} // Internal
} // ClangCodeModel

#endif // DEPENDENCYGRAPH_H
