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
// \file    qanStyleManager.h
// \author  benoit@destrat.io
// \date    2015 06 05
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QSortFilterProxyModel>
#include <QQuickImageProvider>

// QuickContainers headers
#include "./quickcontainers/qcmContainer.h"

// QuickQanava headers
#include "./qanStyle.h"

namespace qan { // ::qan

class Graph;

/*! \brief Manage node/edge/group styles in a qan::Graph.
 *
 * \nosubgrouping
 */
class StyleManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    /*! \name Style Object Management *///-------------------------------------
    //@{
public:
    //! Style manager must be initialized with a valid graph.
    explicit StyleManager( QObject* parent = nullptr );
    virtual ~StyleManager( );
    StyleManager( const StyleManager& ) = delete;
public:
    //! Clear this manager from all its content (default node and edge styles are cleared too).
    Q_INVOKABLE virtual void    clear();
    //@}
    //-------------------------------------------------------------------------

    /*! \name Style Management *///--------------------------------------------
    //@{
public:
    using StyleComponentMap = QMap<qan::Style*, QPointer<QQmlComponent>>;

    Q_INVOKABLE void            setStyleComponent(qan::Style* style, QQmlComponent* component) noexcept;
    Q_INVOKABLE QQmlComponent*  getStyleComponent(qan::Style* style) noexcept;
private:
    StyleComponentMap           _styleComponentMap;

public:
    //! Set style \c nodeStyle a the default style for a specific class of nodes \c delegate.
    void                            setNodeStyle(QQmlComponent* delegate, qan::NodeStyle* nodeStyle);

    //! Get the style for a specific node \c delegate, if no such style exist, return default node style.
    qan::NodeStyle*                 getNodeStyle(QQmlComponent* delegate);

    using DelegateNodeStyleMap = QMap<QQmlComponent*, qan::NodeStyle*>;
    const DelegateNodeStyleMap&     getNodeStyles() const noexcept { return _nodeStyles; }
private:
    DelegateNodeStyleMap            _nodeStyles;

public:
    //! Set style \c defaultEdgeStyle a the default style for a specific class of edge \c delegate.
    void                            setEdgeStyle(QQmlComponent* delegate, qan::EdgeStyle* edgeStyle);

    //! Get the default style for a specific edge \c delegate, if no such style exist, return default node style.
    qan::EdgeStyle*                 getEdgeStyle(QQmlComponent* delegate);

    Q_INVOKABLE qan::EdgeStyle*     createEdgeStyle();

    using DelegateEdgeStyleMap = QMap<QQmlComponent*, qan::EdgeStyle*>;
    const DelegateEdgeStyleMap&     getEdgeStyles() const noexcept { return  _edgeStyles; }
private:
    DelegateEdgeStyleMap            _edgeStyles;

public:
    using ObjectVectorModel = qcm::Container<QVector, QObject*>;

    Q_PROPERTY(QAbstractItemModel* styles READ qmlGetStyles CONSTANT FINAL)
    inline QAbstractItemModel*      qmlGetStyles() noexcept { return _styles.model(); }
    inline const ObjectVectorModel& getStyles() const noexcept { return _styles; }
private:
    //! Styles containers (contain all style for both edges and nodes).
    ObjectVectorModel               _styles;
public:
    Q_INVOKABLE qan::Style*         getStyleAt(int s);
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::StyleManager)
