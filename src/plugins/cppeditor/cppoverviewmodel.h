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

#include <utils/dropsupport.h>
#include <utils/treemodel.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>

#include <QSharedPointer>

#include <utility>

namespace Utils {
class LineColumn;
class Link;
} // namespace Utils

namespace CppEditor::Internal {
class SymbolItem;

class OverviewModel : public Utils::TreeModel<>
{
    Q_OBJECT

public:
    explicit OverviewModel(QObject *parent = nullptr);

    enum Role {
        FileNameRole = Qt::UserRole + 1,
        LineNumberRole
    };

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDragActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    void update(CPlusPlus::Document::Ptr doc);

    int editorRevision();

    bool isGenerated(const QModelIndex &sourceIndex) const;
    Utils::Link linkFromIndex(const QModelIndex &sourceIndex) const;
    Utils::LineColumn lineColumnFromIndex(const QModelIndex &sourceIndex) const;
    using Range = std::pair<Utils::LineColumn, Utils::LineColumn>;
    Range rangeFromIndex(const QModelIndex &sourceIndex) const;

private:
    void rebuild();
    CPlusPlus::Symbol *symbolFromIndex(const QModelIndex &index) const;
    int globalSymbolCount() const;
    CPlusPlus::Symbol *globalSymbolAt(int index) const;
    void buildTree(SymbolItem *root, bool isRoot);

private:
    CPlusPlus::Document::Ptr m_cppDocument;
    CPlusPlus::Overview m_overview;
    friend class SymbolItem;
    QTimer *m_updateTimer = nullptr;
};

} // namespace CppEditor::Internal
