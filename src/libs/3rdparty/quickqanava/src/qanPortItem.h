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
// \file	qanPortItem.h
// \author	benoit@destrat.io
// \date	2017 08 10
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QQuickItem>
#include <QPointF>
#include <QPolygonF>
#include <QDrag>
#include <QPointer>

// QuickQanava headers
#include "./qanNodeItem.h"
#include "./qanEdgeItem.h"

namespace qan { // ::qan

/*! \brief Model a visual port where edge could be binded.
 *
 * \warning Do not modify \c connectable property directly for qan::PortItem, it is set
 * automatically in setType().
 *
 * \nosubgrouping
 */
class PortItem : public qan::NodeItem
{
    /*! \name Dock Object Management *///--------------------------------------
    //@{
    Q_OBJECT
    QML_ELEMENT
public:
    //! PortItem constructor.
    explicit PortItem(QQuickItem* parent = nullptr);
    virtual ~PortItem() override = default;
    PortItem(const PortItem&) = delete;
    PortItem& operator=(const PortItem&) = delete;
    PortItem(PortItem&&) = delete;
    PortItem& operator=(PortItem&&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Port Properties Management *///----------------------------------
    //@{
public:
    //! Port type, either IN or OUT port.
    enum class Type {
        //! Port accept in and out connections.
        InOut,
        //! Port accept only in connections (default).
        In,
        //! Port accept only out connections.
        Out
    };
    Q_ENUM(Type)

    //! Port acces either multiple or single in or out connections (depending on port \c type).
    enum class Multiplicity {
        //! Port accept only on in or out connections (default).
        Multiple,
        //! Port accept only in connections (default).
        Single
    };
    Q_ENUM(Multiplicity)

public:
    //! \copydoc Node in or out or in/out (default to in/out).
    Q_PROPERTY(Type type READ getType CONSTANT FINAL)
    //! \copydoc Type
    inline Type      getType() const noexcept { return _type; }
    //! \copydoc Type
    auto             setType(Type type) noexcept -> void;
private:
    //! \copydoc Type
    Type            _type{Type::InOut};

public:
    //! \copydoc Multiplicity
    Q_PROPERTY(Multiplicity multiplicity READ getMultiplicity WRITE setMultiplicity NOTIFY multiplicityChanged FINAL)
    //! \copydoc Multiplicity
    inline Multiplicity getMultiplicity() const noexcept { return _multiplicity; }
    //! \copydoc Multiplicity
    auto                setMultiplicity(Multiplicity multiplicity) noexcept -> void;
private:
    //! \copydoc Multiplicity
    Multiplicity        _multiplicity{Multiplicity::Multiple};
signals:
    void                multiplicityChanged();

public:
    //!
    Q_PROPERTY(qan::NodeItem::Dock dockType READ getDockType NOTIFY dockTypeChanged FINAL)
    inline NodeItem::Dock   getDockType() const noexcept { return _dockType; }
    void                    setDockType(NodeItem::Dock dockType) noexcept;
private:
    qan::NodeItem::Dock     _dockType{NodeItem::Dock::Left};
signals:
    void                    dockTypeChanged();

public:
    Q_PROPERTY(QString label READ getLabel WRITE setLabel NOTIFY labelChanged FINAL)
    void            setLabel( const QString& label ) noexcept;
    inline QString  getLabel() const noexcept { return _label; }
private:
    QString         _label = "";
signals:
    void            labelChanged();

public:
    void            setId(const QString& id) noexcept { _id = id; }
    const QString&  getId() const noexcept { return _id; }
private:
    QString         _id = "";

public:
    using EdgeItems =   qcm::Container<QVector, qan::EdgeItem*>;

    void                addInEdgeItem(qan::EdgeItem& inEdgeItem) noexcept;
    void                addOutEdgeItem(qan::EdgeItem& outEdgeItem) noexcept;

    EdgeItems&          getInEdgeItems() noexcept { return _inEdgeItems; }
    const EdgeItems&    getInEdgeItems() const noexcept { return _inEdgeItems; }

    EdgeItems&          getOutEdgeItems() noexcept { return _outEdgeItems; }
    const EdgeItems&    getOutEdgeItems() const noexcept { return _outEdgeItems; }

protected:
    EdgeItems           _inEdgeItems;
    EdgeItems           _outEdgeItems;

    //! Used internally to automatically monitor in/out edges items destruction.
    void                onEdgeItemDestroyed(QObject* obj);

public:
    //! Force input/output edge update.
    Q_INVOKABLE virtual void updateEdges();
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::PortItem)
Q_DECLARE_METATYPE(qan::PortItem::Type)
Q_DECLARE_METATYPE(qan::PortItem::Multiplicity)
