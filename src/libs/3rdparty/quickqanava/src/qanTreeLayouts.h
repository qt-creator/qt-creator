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

#pragma once

// Qt headers
#include <QString>
#include <QQuickItem>
#include <QQmlParserStatus>
#include <QSharedPointer>
#include <QAbstractListModel>

// QuickQanava headers
#include "./qanGraph.h"


namespace qan { // ::qan

/*! \brief
 * \nosubgrouping
 */
class NaiveTreeLayout : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    /*! \name NaiveTreeLayout Object Management *///---------------------------
    //@{
public:
    explicit NaiveTreeLayout(QObject* parent = nullptr) noexcept;
    virtual ~NaiveTreeLayout() override;
    NaiveTreeLayout(const NaiveTreeLayout&) = delete;
    NaiveTreeLayout& operator=(const NaiveTreeLayout&) = delete;
    NaiveTreeLayout(NaiveTreeLayout&&) = delete;
    NaiveTreeLayout& operator=(NaiveTreeLayout&&) = delete;

public:
    // FIXME #228
    void                layout(qan::Node& root) noexcept;

    //! QML invokable version of layout().
    Q_INVOKABLE void    layout(qan::Node* root) noexcept;
    //@}
    //-------------------------------------------------------------------------
};


/*! \brief Layout nodes randomly inside a bounding rect.
 * \nosubgrouping
 */
class RandomLayout : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    /*! \name RandomLayout Object Management *///-----------------------------
    //@{
public:
    explicit RandomLayout(QObject* parent = nullptr) noexcept;
    virtual ~RandomLayout() override;
    RandomLayout(const RandomLayout&) = delete;
    RandomLayout& operator=(const RandomLayout&) = delete;
    RandomLayout(RandomLayout&&) = delete;
    RandomLayout& operator=(RandomLayout&&) = delete;

public:
    //! \copydoc getLayoutRect()
    Q_PROPERTY(QRectF layoutRect READ getLayoutRect WRITE setLayoutRect NOTIFY layoutRectChanged FINAL)
    //! \copydoc getLayoutRect()
    bool            setLayoutRect(QRectF layoutRect) noexcept;
    //! \copydoc getLayoutRect()
    const QRectF    getLayoutRect() const noexcept;
protected:
    //! \copydoc getLayoutRect()
    QRectF          _layoutRect = QRectF{};
signals:
    //! \copydoc getLayoutRect()
    void            layoutRectChanged();

public:
    /*! \brief Apply a random layout fitting nodes positions inside \c layoutRect.
     * If \c layoutRect is empty, generate a 1000x1000 default rect around \c root position.
     */
    void                layout(qan::Node& root) noexcept;

    //! QML invokable version of layout().
    Q_INVOKABLE void    layout(qan::Node* root) noexcept;
    //@}
    //-------------------------------------------------------------------------
};


/*! \brief Org chart naive recursive variant of Reingold-Tilford algorithm with shifting.
 *
 * This algorithm layout tree in an "Org chart" fashion using no space optimization,
 * respecting node ordering and working for n-ary trees.
 *
 * \note This layout does not enforces that the input graph is a tree, laying out
 * a non-tree graph might lead to infinite recursion.
 *
 * \nosubgrouping
 */
class OrgTreeLayout : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    /*! \name OrgTreeLayout Object Management *///-----------------------------
    //@{
public:
    explicit OrgTreeLayout(QObject* parent = nullptr) noexcept;
    virtual ~OrgTreeLayout() override;
    OrgTreeLayout(const OrgTreeLayout&) = delete;
    OrgTreeLayout& operator=(const OrgTreeLayout&) = delete;
    OrgTreeLayout(OrgTreeLayout&&) = delete;
    OrgTreeLayout& operator=(OrgTreeLayout&&) = delete;

public:
    //! Define layout orentation, modify before a call to layout(), default to Vertical.
    enum class LayoutOrientation : unsigned int {
        //! Undefined.
        Undefined = 0,
        //! Vertical tree layout.
        Vertical = 2,
        //! Horizontal tree layout.
        Horizontal = 4,
        //! Mixed t0ree layout (ie vertical, but horizontal for leaf nodes).
        Mixed = 8
    };
    Q_ENUM(LayoutOrientation)

    //! \copydoc LayoutOrientation
    Q_PROPERTY(LayoutOrientation layoutOrientation READ getLayoutOrientation WRITE setLayoutOrientation NOTIFY layoutOrientationChanged FINAL)
    //! \copydoc LayoutOrientation
    bool                    setLayoutOrientation(LayoutOrientation layoutOrientation) noexcept;
    //! \copydoc LayoutOrientation
    LayoutOrientation       getLayoutOrientation() noexcept;
    //! \copydoc LayoutOrientation
    const LayoutOrientation getLayoutOrientation() const noexcept;
protected:
    //! \copydoc LayoutOrientation
    LayoutOrientation       _layoutOrientation = LayoutOrientation::Vertical;
signals:
    //! \copydoc LayoutOrientation
    void                    layoutOrientationChanged();

public:
    /*! \brief Apply a vertical "organisational chart tree layout algorithm" to subgraph \c root.
     *
     * OrgChart layout _will preserve_ node orders.
     *
     * This naive implementation is recursive and not "space optimal" while it run in O(n),
     * n beeing the number of nodes in root "tree subgraph".
     *
     * \note \c root must be a tree subgraph, this method will not enforce this condition,
     * running this algorithm on a non tree subgraph might lead to inifinite recursions or
     * invalid layouts.
     */
    void                layout(qan::Node& root, qreal xSpacing = 25., qreal ySpacing = 25.) noexcept;

    //! QML invokable version of layout().
    Q_INVOKABLE void    layout(qan::Node* root, qreal xSpacing = 25., qreal ySpacing = 25.) noexcept;
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::OrgTreeLayout)

