/*
 Copyright (c) 2008-2024, Benoit AUTHEMAN All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author or Destrat.io nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the QuickQanava software library.
//
// \file	qanOrgTreeLayout.h
// \author	benoit@destrat.io
// \date	2024 08 13
//-----------------------------------------------------------------------------

// Std headers
#include <queue>
#include <unordered_set>

// Qt headers
#include <QQmlProperty>
#include <QVariant>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QRandomGenerator>

// QuickQanava headers
#include "./qanTreeLayouts.h"


namespace qan { // ::qan

/* NaiveTreeLayout Object Management *///--------------------------------------
NaiveTreeLayout::NaiveTreeLayout(QObject* parent) noexcept :
    QObject{parent}
{
}
NaiveTreeLayout::~NaiveTreeLayout() { }

void    NaiveTreeLayout::layout(qan::Node& root) noexcept
{
    // Pre-condition: root must be a tree subgraph, this is not enforced in this methodod.

    // Algorithm:
    // 1. Use BFS to generate an array of nodes "level by level".
    // 2. Layout the tree bottom up; for every level, bottom up:
    // 2.1 Layout the node at y(level) position.
    // 2.2 For mid level, shift nodes x position to align right subgraph on it's previous
    //     node subgraph (magic happen here: use a shifting heuristic !)
    // 2.3 Align node on it's sub graph (according to input configuration align left or center)
    // 3. Shift the tree to align root to it's original position.

    const auto collectBFS = [](qan::Node* root) -> std::vector<std::vector<qan::Node*>> {
        std::vector<std::vector<qan::Node*>> r;
        if (root == nullptr)
            return r;

        // <-- hand tuned ChatGPT code
        std::queue<std::pair<Node*, int>> nodeLevelqueue;
        std::unordered_set<Node*> visited;

        nodeLevelqueue.push({root, 0});
        visited.insert(root);

        while (!nodeLevelqueue.empty()) {
            auto [current, level] = nodeLevelqueue.front();
            nodeLevelqueue.pop();

            if (r.size() <= level)
                r.resize(level + 1);    // Resize() initialize new items
            r[level].push_back(current);
            for (auto child : current->get_out_nodes()) {             // Enqueue unvisited children with their level
                if (visited.find(child) == visited.end()) {
                    nodeLevelqueue.push({child, level + 1});
                    visited.insert(child);
                }
            }
        }
        // <-- ChatGPT code
        return r;
    };

    // 1. BFS
    const auto levels = collectBFS(&root);

    // Debug
    int l = 0;
    for (const auto& level: levels) {
        std::cerr << l++ << ": ";
        for (const auto node: level)
            std::cerr << node->getLabel().toStdString() << "\t";
        std::cerr << std::endl;
    }

    // 2.
    if (levels.size() <= 1)   // Can't layout a tree with less than 2 levels
        return;
    const double xSpacing = 25.;
    const double ySpacing = 125.;
    for (int level = levels.size() - 1; level >= 0; level--) {
        auto nodes = levels[level];

        // 2.1
        const double y = level * ySpacing; // FIXME, be smarter on shift here...

        // 2.2
        double x = 0.;
        for (const auto node: nodes) {
            node->getItem()->setX(x);
            node->getItem()->setY(y);
            x += node->getItem()->getBoundingShape().boundingRect().width() + xSpacing;
        }
    }

    // FIXME centering in another pass...
}

void    NaiveTreeLayout::layout(qan::Node* root) noexcept
{
    qWarning() << "qan::NaiveTreeLayout::layout(): root=" << root;
    if (root != nullptr)
        layout(*root);
}
//-----------------------------------------------------------------------------


/* OrgTreeLayout Object Management *///----------------------------------------
RandomLayout::RandomLayout(QObject* parent) noexcept :
    QObject{parent}
{
}
RandomLayout::~RandomLayout() { }

bool    RandomLayout::setLayoutRect(QRectF layoutRect) noexcept
{
    _layoutRect = layoutRect;
    emit layoutRectChanged();
    return true;
}
const QRectF    RandomLayout::getLayoutRect() const noexcept { return _layoutRect; }

void    RandomLayout::layout(qan::Node& root) noexcept
{
    // In nodes, out nodes, adjacent nodes ?
    const auto graph = root.getGraph();
    if (graph == nullptr)
        return;
    if (root.getItem() == nullptr)
        return;

    // Generate a 1000x1000 layout rect centered on root if the user has not specified one
    const auto rootPosition = root.getItem()->position();
    const auto layoutRect = _layoutRect.isEmpty() ? QRectF{rootPosition.x() - 500, rootPosition.y() - 500, 1000., 1000.} :
                                                    _layoutRect;

    auto outNodes = graph->collectSubNodes(QVector<qan::Node*>{&root}, false);
    outNodes.insert(&root);
    for (auto n : outNodes) {
        auto node = const_cast<qan::Node*>(n);
        if (node->getItem() == nullptr)
            continue;
        const auto nodeBr = node->getItem()->boundingRect();
        qreal maxX = layoutRect.width() - nodeBr.width();       // Generate and set random x and y positions
        qreal maxY = layoutRect.height() - nodeBr.height();     // within available layoutRect area
        node->getItem()->setX(QRandomGenerator::global()->bounded(maxX) + layoutRect.left());
        node->getItem()->setY(QRandomGenerator::global()->bounded(maxY) + layoutRect.top());
    }
}

void    RandomLayout::layout(qan::Node* root) noexcept
{
    if (root != nullptr)
        layout(*root);
}
//-----------------------------------------------------------------------------


/* OrgTreeLayout Object Management *///----------------------------------------
OrgTreeLayout::OrgTreeLayout(QObject* parent) noexcept :
    QObject{parent}
{
}
OrgTreeLayout::~OrgTreeLayout() { }

bool    OrgTreeLayout::setLayoutOrientation(OrgTreeLayout::LayoutOrientation layoutOrientation) noexcept {
    if (_layoutOrientation != layoutOrientation) {
        _layoutOrientation = layoutOrientation;
        emit layoutOrientationChanged();
        return true;
    }
    return false;
}
OrgTreeLayout::LayoutOrientation        OrgTreeLayout::getLayoutOrientation() noexcept { return _layoutOrientation; }
const OrgTreeLayout::LayoutOrientation  OrgTreeLayout::getLayoutOrientation() const noexcept { return _layoutOrientation; }


void    OrgTreeLayout::layout(qan::Node& root, qreal xSpacing, qreal ySpacing) noexcept
{
    // Note: Recursive variant of Reingold-Tilford algorithm with naive shifting (ie shifting
    // based on the less space efficient sub tree bounding rect intersection...)

    // Pre-condition: root must be a tree subgraph, this is not enforced in this algorithm,
    // any circuit will lead to intinite recursion...

    // Algorithm:
        // Traverse graph DFS aligning child nodes vertically
        // At a given level: `shift` next node according to previous node sub-tree BR
    auto layoutVert_rec = [xSpacing, ySpacing](auto&& self, auto& childNodes, QRectF br) -> QRectF {
        const auto x = br.right() + xSpacing;
        for (auto child: childNodes) {
            child->getItem()->setX(x);
            child->getItem()->setY(br.bottom() + ySpacing);
            // Take into account this level maximum width
            br = br.united(child->getItem()->boundingRect().translated(child->getItem()->position()));
            const auto childBr = self(self, child->get_out_nodes(), br);
            br.setBottom(childBr.bottom()); // Note: Do not take full child BR into account to avoid x drifting
        }
        return br;
    };

    auto layoutHoriz_rec = [xSpacing, ySpacing](auto&& self, auto& childNodes, QRectF br) -> QRectF {
        const auto y = br.bottom() + ySpacing;
        for (auto child: childNodes) {
            child->getItem()->setX(br.right() + xSpacing);
            child->getItem()->setY(y);
            // Take into account this level maximum width
            br = br.united(child->getItem()->boundingRect().translated(child->getItem()->position()));
            const auto childBr = self(self, child->get_out_nodes(), br);
            br.setRight(childBr.right()); // Note: Do not take full child BR into account to avoid x drifting
        }
        return br;
    };

    auto layoutMixed_rec = [xSpacing, ySpacing, layoutHoriz_rec](auto&& self, auto& childNodes, QRectF br) -> QRectF {
        auto childsAreLeafs = true;
        for (const auto child: childNodes)
            if (child->get_out_nodes().size() != 0) {
                childsAreLeafs = false;
                break;
            }
        if (childsAreLeafs)
            return layoutHoriz_rec(self, childNodes, br);
        else {
            const auto x = br.right() + xSpacing;
            for (auto child: childNodes) {
                child->getItem()->setX(x);
                child->getItem()->setY(br.bottom() + ySpacing);
                // Take into account this level maximum width
                br = br.united(child->getItem()->boundingRect().translated(child->getItem()->position()));
                const auto childBr = self(self, child->get_out_nodes(), br);
                br.setBottom(childBr.bottom()); // Note: Do not take full child BR into account to avoid x drifting
            }
        }
        return br;
    };

    // Note: QQuickItem boundingRect() is in item local CS, translate to "scene" CS.
    switch (getLayoutOrientation()) {
    case LayoutOrientation::Undefined: return;
    case LayoutOrientation::Vertical:
        layoutVert_rec(layoutVert_rec, root.get_out_nodes(),
                       root.getItem()->boundingRect().translated(root.getItem()->position()));
        break;
    case LayoutOrientation::Horizontal:
        layoutHoriz_rec(layoutHoriz_rec, root.get_out_nodes(),
                        root.getItem()->boundingRect().translated(root.getItem()->position()));
        break;
    case LayoutOrientation::Mixed:
        layoutMixed_rec(layoutMixed_rec, root.get_out_nodes(),
                        root.getItem()->boundingRect().translated(root.getItem()->position()));
        break;
    }
}

void    OrgTreeLayout::layout(qan::Node* root, qreal xSpacing, qreal ySpacing) noexcept
{
    if (root != nullptr)
        layout(*root, xSpacing, ySpacing);
}
//-----------------------------------------------------------------------------

} // ::qan
