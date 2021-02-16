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

#include <cplusplus/CppDocument.h>

#include <QSharedPointer>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QStandardItem)

namespace ClassView {
namespace Internal {

class ParserTreeItemPrivate;

class ParserTreeItem
{
public:
    using ConstPtr = QSharedPointer<const ParserTreeItem>;

public:
    ParserTreeItem();
    ParserTreeItem(const Utils::FilePath &projectFilePath);
    ParserTreeItem(const QHash<SymbolInformation, ConstPtr> &children);
    ~ParserTreeItem();

    static ConstPtr parseDocument(const CPlusPlus::Document::Ptr &doc);
    static ConstPtr mergeTrees(const Utils::FilePath &projectFilePath, const QList<ConstPtr> &docTrees);

    Utils::FilePath projectFilePath() const;
    QSet<SymbolLocation> symbolLocations() const;
    ConstPtr child(const SymbolInformation &inf) const;
    int childCount() const;

    // Make sure that below two methods are called only from the GUI thread
    bool canFetchMore(QStandardItem *item) const;
    void fetchMore(QStandardItem *item) const;

    void debugDump(int indent = 0) const;

private:
    friend class ParserTreeItemPrivate;
    ParserTreeItemPrivate *d;
};

} // namespace Internal
} // namespace ClassView

Q_DECLARE_METATYPE(ClassView::Internal::ParserTreeItem::ConstPtr)
