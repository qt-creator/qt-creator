/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "classviewsymbollocation.h"
#include "classviewsymbolinformation.h"

#include <QSharedPointer>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QStandardItem)

namespace ClassView {
namespace Internal {

class ParserTreeItemPrivate;

class ParserTreeItem
{
public:
    typedef QSharedPointer<ParserTreeItem> Ptr;
    typedef QSharedPointer<const ParserTreeItem> ConstPtr;

public:
    ParserTreeItem();
    ~ParserTreeItem();

    void copyTree(const ParserTreeItem::ConstPtr &from);

    void copy(const ParserTreeItem::ConstPtr &from);

    void addSymbolLocation(const SymbolLocation &location);

    void addSymbolLocation(const QSet<SymbolLocation> &locations);

    void removeSymbolLocation(const SymbolLocation &location);

    void removeSymbolLocations(const QSet<SymbolLocation> &locations);

    QSet<SymbolLocation> symbolLocations() const;

    void appendChild(const ParserTreeItem::Ptr &item, const SymbolInformation &inf);

    void removeChild(const SymbolInformation &inf);

    ParserTreeItem::Ptr child(const SymbolInformation &inf) const;

    int childCount() const;

    void convertTo(QStandardItem *item) const;

    // additional properties
    //! Assigned icon
    QIcon icon() const;

    //! Set an icon for this tree node
    void setIcon(const QIcon &icon);

    void add(const ParserTreeItem::ConstPtr &target);

    bool canFetchMore(QStandardItem *item) const;

    void fetchMore(QStandardItem *item) const;

    void debugDump(int ident = 0) const;

protected:
    ParserTreeItem &operator=(const ParserTreeItem &other);

private:
    typedef QHash<SymbolInformation, ParserTreeItem::Ptr>::const_iterator CitSymbolInformations;
    //! Private class data pointer
    ParserTreeItemPrivate *d;
};

} // namespace Internal
} // namespace ClassView
