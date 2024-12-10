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
// \file    qanGroupItem.h
// \author  benoit@destrat.io
// \date    2017 03 02
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QQuickItem>
#include <QPointF>
#include <QPolygonF>

// QuickQanava headers
#include "./qanNodeItem.h"
#include "./qanGroup.h"

namespace qan { // ::qan

class Graph;

/*! \brief Model a visual group of nodes.
 *
 * \note Groups are styled with qan::NodeStyle.
 * \warning \c objectName property is set to "qan::GroupItem" and should not be changed in subclasses.
 *
 * \nosubgrouping
 */
class GroupItem : public qan::NodeItem
{
    /*! \name Group Object Management *///-------------------------------------
    //@{
    Q_OBJECT
    QML_ELEMENT
public:
    //! Group constructor.
    explicit GroupItem(QQuickItem* parent = nullptr);
    virtual ~GroupItem() override = default;
    GroupItem(const GroupItem&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Topology Management *///-----------------------------------------
    //@{
public:
    Q_PROPERTY(qan::Group* group READ getGroup CONSTANT FINAL)
    auto            getGroup() noexcept -> qan::Group*;
    auto            getGroup() const noexcept -> const qan::Group*;
    virtual bool    setGroup(qan::Group* group) noexcept;
protected:
    QPointer<qan::Group> _group{nullptr};

public:
    //! Utility function to ease initialization from c++, call setX(), setY(), setWidth() and setHEight() with the content of \c rect bounding rect.
    auto        setRect(const QRectF& r) noexcept -> void;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Collapse / Edition Management *///-------------------------------
    //@{
protected:
    virtual void    setCollapsed(bool collapsed) noexcept override;

public:
    //! \copydoc getExpandButtonVisible()
    Q_PROPERTY(bool expandButtonVisible READ getExpandButtonVisible WRITE setExpandButtonVisible NOTIFY expandButtonVisibleChanged FINAL)
    //! \copydoc getExpandButtonVisible()
    void            setExpandButtonVisible(bool expandButtonVisible);
    //! \brief Show / hide the group expand / collapse button.
    bool            getExpandButtonVisible() const;
private:
    //! \copydoc getExpandButtonVisible()
    bool            _expandButtonVisible = true;
signals:
    //! \copydoc getExpandButtonVisible()
    void            expandButtonVisibleChanged();

public:
    //! \copydoc getLabelEditorVisible()
    Q_PROPERTY(bool labelEditorVisible READ getLabelEditorVisible WRITE setLabelEditorVisible NOTIFY labelEditorVisibleChanged FINAL)
    //! \copydoc getLabelEditorVisible()
    void            setLabelEditorVisible(bool labelEditorVisible);
    //! \brief True when the group label in group editor is beeing edited.
    bool            getLabelEditorVisible() const;
private:
    //! \copydoc getLabelEditorVisible()
    bool            _labelEditorVisible = false;
signals:
    //! \copydoc getLabelEditorVisible()
    void            labelEditorVisibleChanged();
    //@}
    //-------------------------------------------------------------------------

    /*! \name Dragging Support Management *///---------------------------------
    //@{
public:
    //! Define a group "dragging" policy: either only from group header or content (default to Header, can be or'ed).
    enum class DragPolicy : unsigned int {
        //! Undefined / No dragging.
        Undefined = 0,
        //! Allow dragging group in the group header.
        Header = 2,
        //! Allow dragging group in the group content.
        Container = 4
    };
    Q_ENUM(DragPolicy)
    //! \copydoc DragPolicy
    Q_PROPERTY(DragPolicy dragPolicy READ getDragPolicy WRITE setDragPolicy NOTIFY dragPolicyChanged FINAL)
    //! \copydoc DragPolicy
    virtual bool        setDragPolicy(DragPolicy dragPolicy) noexcept;
    //! \copydoc DragPolicy
    DragPolicy          getDragPolicy() noexcept;
    //! \copydoc DragPolicy
    const DragPolicy    getDragPolicy() const noexcept;
protected:
    //! \copydoc DragPolicy
    DragPolicy  _dragPolicy = DragPolicy::Header;
signals:
    //! \copydoc DragPolicy
    void        dragPolicyChanged();


protected slots:
    //! Group is monitored for position change, since group's nodes edges should be updated manually in that case.
    void            groupMoved();

public:
    /*! \brief Configure \c nodeItem in this group item (modify target item parenthcip, but keep same visual position).
     */
    virtual void    groupNodeItem(qan::NodeItem* nodeItem, qan::TableCell* groupCell, bool transform = true);

    //! Configure \c nodeItem outside this group item (modify parentship, keep same visual position).
    virtual void    ungroupNodeItem(qan::NodeItem* nodeItem, bool transform = true);

    //! Call at the beginning of another group or node hover operation on this group (usually trigger a visual change to notify user that insertion is possible trought DND).
    inline void     proposeNodeDrop() noexcept { emit nodeDragEnter( ); }

    //! End an operation started with proposeNodeDrop().
    inline void     endProposeNodeDrop() noexcept { emit nodeDragLeave( ); }

public:
    /*! \brief Should be set from the group concrete QML component to indicate the group content item (otherwise, this will be used).
     *
     * For example, if the actual container for node is a child of the concrete group component (most of the time, an Item or a Rectangle, use the
     * following code to set 'container' property:
     *
     * \code
     * Qan.GroupItem {
     *   id: groupItem
     *   default property alias children : content
     *   container: content
     *   Item {
     *     id: content
     *     // ...
     *   }
     *   container = content
     * }
     * \endcode
     */
    Q_PROPERTY(QQuickItem* container READ getContainer WRITE setContainer NOTIFY containerChanged FINAL)
    virtual bool            setContainer(QQuickItem* container) noexcept;
    QQuickItem*             getContainer() noexcept;
    const QQuickItem*       getContainer() const noexcept;
protected:
    QPointer<QQuickItem>    _container = nullptr;
signals:
    void                    containerChanged();

public:
    /*! \brief In strict drop mode (default), a node bounding rect must be fully inside the group until it can be dropped.
     *
     *  With `strictMode` to false, only the top left corner of node must be inside group to trigger
     *  a drop proposition. Actually used internally by the library for `qan::TableGroup` where only part
     *  of a node could be inside the group since it later resized to fit a cell. Do not modify until you know
     *  what you are doing.
     */
    Q_PROPERTY(bool strictDrop READ getStrictDrop WRITE setStrictDrop NOTIFY strictDropChanged FINAL)
    void            setStrictDrop(bool strictDrop) noexcept;
    bool            getStrictDrop() const noexcept;
private:
    bool            _strictDrop = true;
signals:
    void            strictDropChanged();

signals:
    //! Emitted whenever a dragged node enter the group area (could be usefull to hilight it in a qan::Group concrete QML component).
    void            nodeDragEnter();
    //! Emitted whenever a dragged node leave the group area (could be usefull to hilight it in a qan::Group concrete QML component).
    void            nodeDragLeave();

protected:
    virtual void    mouseDoubleClickEvent(QMouseEvent* event ) override;
    virtual void    mousePressEvent(QMouseEvent* event ) override;

signals:
    //! Emitted whenever the group is clicked (even at the start of a dragging operation).
    void    groupClicked(qan::GroupItem* group, QPointF p);
    //! Emitted whenever the group is double clicked.
    void    groupDoubleClicked(qan::GroupItem* group, QPointF p);
    //! Emitted whenever the group is right clicked.
    void    groupRightClicked(qan::GroupItem* group, QPointF p);
    //@}
    //-------------------------------------------------------------------------
};

// Define the bitwise AND operator for MyEnum
inline GroupItem::DragPolicy operator&(GroupItem::DragPolicy lhs, GroupItem::DragPolicy rhs) {
    return static_cast<GroupItem::DragPolicy>(static_cast<unsigned int>(lhs) & static_cast<unsigned int>(rhs));
}

} // ::qan

QML_DECLARE_TYPE(qan::GroupItem)
