/**************************************************************************
**
** Copyright (c) 2013 Denis Mingulov
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

#ifndef CLASSVIEWPARSERTREEITEM_H
#define CLASSVIEWPARSERTREEITEM_H

#include "classviewsymbollocation.h"
#include "classviewsymbolinformation.h"

#include <QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QStandardItem)

namespace ClassView {
namespace Internal {

class ParserTreeItemPrivate;

/*!
   \class ParserTreeItem
   \brief Item for the internal Class View Tree

   Item for Class View Tree.
   Not virtual - to speed up its work.
 */

class ParserTreeItem
{
public:
    typedef QSharedPointer<ParserTreeItem> Ptr;
    typedef QSharedPointer<const ParserTreeItem> ConstPtr;

public:
    ParserTreeItem();
    ~ParserTreeItem();

    /*!
       \brief Copy content of \a from item with children to this one.
       \param from 'From' item
     */
    void copyTree(const ParserTreeItem::ConstPtr &from);

    /*!
       \brief Copy of \a from item to this one.
       \param from 'From' item
     */
    void copy(const ParserTreeItem::ConstPtr &from);

    /*!
       \brief Add information about symbol location
       \param location Filled \a SymbolLocation struct with a correct information
       \sa SymbolLocation, removeSymbolLocation, symbolLocations
     */
    void addSymbolLocation(const SymbolLocation &location);

    /*!
       \brief Add information about symbol locations
       \param locations Filled \a SymbolLocation struct with a correct information
       \sa SymbolLocation, removeSymbolLocation, symbolLocations
     */
    void addSymbolLocation(const QSet<SymbolLocation> &locations);

    /*!
       \brief Remove information about symbol location
       \param location Filled \a SymbolLocation struct with a correct information
       \sa SymbolLocation, addSymbolLocation, symbolLocations
     */
    void removeSymbolLocation(const SymbolLocation &location);

    /*!
       \brief Remove information about symbol locations
       \param locations Filled \a SymbolLocation struct with a correct information
       \sa SymbolLocation, addSymbolLocation, symbolLocations
     */
    void removeSymbolLocations(const QSet<SymbolLocation> &locations);

    /*!
       \brief Get information about symbol positions
       \sa SymbolLocation, addSymbolLocation, removeSymbolLocation
     */
    QSet<SymbolLocation> symbolLocations() const;

    /*!
       \brief Append child
       \param item Child item
       \param inf Symbol information
     */
    void appendChild(const ParserTreeItem::Ptr &item, const SymbolInformation &inf);

    /*!
       \brief Remove child
       \param inf SymbolInformation which has to be removed
     */
    void removeChild(const SymbolInformation &inf);

    /*!
       \brief Get an item
       \param inf Symbol information about needed child
       \return Found child
     */
    ParserTreeItem::Ptr child(const SymbolInformation &inf) const;

    /*!
       \brief How many children
       \return Amount of chilren
     */
    int childCount() const;

    /*!
       \brief Append this item to the \a QStandardIten item
       \param item QStandardItem
       \param recursive Do it recursively for the tree items or not (might be needed for
                        the lazy data population

     */
    void convertTo(QStandardItem *item, bool recursive = true) const;

    // additional properties
    //! Assigned icon
    QIcon icon() const;

    //! Set an icon for this tree node
    void setIcon(const QIcon &icon);

    /*!
       \brief Add an internal state with \a target.
       \param target Item which contains the correct current state
     */
    void add(const ParserTreeItem::ConstPtr &target);

    /*!
       \brief Subtract an internal state with \a target.
       \param target Item which contains the subtrahend
     */
    void subtract(const ParserTreeItem::ConstPtr &target);

    /*!
       \brief Lazy data population for a \a QStandardItemModel
       \param item Item which has to be checked
     */
    bool canFetchMore(QStandardItem *item) const;

    /*!
       \brief Lazy data population for a \a QStandardItemModel
       \param item Item which will be populated (if needed)
     */
    void fetchMore(QStandardItem *item) const;

    /*!
       \brief Debug dump
     */
    void debugDump(int ident = 0) const;

protected:
    ParserTreeItem &operator=(const ParserTreeItem &other);

private:
    //! Private class data pointer
    ParserTreeItemPrivate *d;
};

} // namespace Internal
} // namespace ClassView

#endif // CLASSVIEWPARSERTREEITEM_H
