/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "abstractoverviewmodel.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>

namespace CppTools {

class SymbolItem : public Utils::TreeItem
{
public:
    SymbolItem() = default;
    explicit SymbolItem(CPlusPlus::Symbol *symbol) : symbol(symbol) {}

    QVariant data(int column, int role) const override;

    CPlusPlus::Symbol *symbol = nullptr; // not owned
};


class CPPTOOLS_EXPORT OverviewModel : public AbstractOverviewModel
{
    Q_OBJECT

public:
    void rebuild(CPlusPlus::Document::Ptr doc) override;

    bool isGenerated(const QModelIndex &sourceIndex) const override;
    Utils::Link linkFromIndex(const QModelIndex &sourceIndex) const override;
    Utils::LineColumn lineColumnFromIndex(const QModelIndex &sourceIndex) const override;
    Range rangeFromIndex(const QModelIndex &sourceIndex) const override;

private:
    CPlusPlus::Symbol *symbolFromIndex(const QModelIndex &index) const;
    bool hasDocument() const;
    int globalSymbolCount() const;
    CPlusPlus::Symbol *globalSymbolAt(int index) const;
    void buildTree(SymbolItem *root, bool isRoot);

private:
    CPlusPlus::Document::Ptr _cppDocument;
    CPlusPlus::Overview _overview;
    friend class SymbolItem;
};

} // namespace CppTools
