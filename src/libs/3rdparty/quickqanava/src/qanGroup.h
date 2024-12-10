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
// \file    qanGroup.h
// \author  benoit@destrat.io
// \date    2016 03 22
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QQuickItem>
#include <QPointF>
#include <QPolygonF>

// QuickQanava headers
#include "./qanStyle.h"
#include "./qanNode.h"

Q_MOC_INCLUDE("./qanGraph.h")
Q_MOC_INCLUDE("./qanGroupItem.h")

namespace qan { // ::qan

class Graph;
class GroupItem;

/*! \brief Model a graphics group of nodes.
 *
 * \nosubgrouping
 */
class Group : public qan::Node
{
    /*! \name Group Object Management *///-------------------------------------
    //@{
    Q_OBJECT
public:
    //! Group constructor.
    explicit Group(QObject* parent = nullptr);
    /*! \brief Remove any childs group who have no QQmlEngine::CppOwnership.
     *
     */
    virtual ~Group() override = default;
    Group(const Group&) = delete;

public:
    //! Return true if this group is a table (ie a qan::TableGroup or subclass).
    Q_INVOKABLE virtual bool    isTable() const;

    /*! \brief Collect this group adjacent edges (ie adjacent edges of group and group nodes).
     *
     */
    virtual std::unordered_set<qan::Edge*>  collectAdjacentEdges() const override;

public:
    friend class qan::GroupItem;

    qan::GroupItem*         getGroupItem() noexcept;
    const qan::GroupItem*   getGroupItem() const noexcept;
    virtual void            setItem(qan::NodeItem* item) noexcept override;

public:
    //! Shortcut to getItem()->proposeNodeDrop(), defined only for g++ compatibility to avoid forward template declaration.
    void    itemProposeNodeDrop();
    //! Shortcut to getItem()->endProposeNodeDrop(), defined only for g++ compatibility to avoid forward template declaration.
    void    itemEndProposeNodeDrop();
    //@}
    //-------------------------------------------------------------------------

    /*! \name Group Static Factories *///--------------------------------------
    //@{
public:
    /*! \brief Return the default delegate QML component that should be used to generate group \c item.
     *
     *  \arg engine QML engine used for delegate QML component creation.
     *  \return Default delegate component or nullptr (when nullptr is returned, QuickQanava default to Qan.Group component).
     */
    static  QQmlComponent*      delegate(QQmlEngine& engine, QObject* parent = nullptr) noexcept;

    /*! \brief Return the default style that should be used with qan::Group.
     *
     *  \return Default style or nullptr (when nullptr is returned, qan::StyleManager default group style will be used).
     */
    static  qan::NodeStyle*     style(QObject* parent = nullptr) noexcept;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Group Nodes Management *///--------------------------------------
    //@{
public:
    //! Return true if node \c node is registered in this group, shortcut to gtpo::group<qan::Config>::hasNode().
    Q_INVOKABLE bool    hasNode(const qan::Node* node) const;
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::Group)
