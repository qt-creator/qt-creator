// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/dropsupport.h>
#include <utils/textutils.h>
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

class OutlineModel : public Utils::TreeModel<>
{
    Q_OBJECT

public:
    explicit OutlineModel(QObject *parent = nullptr);

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
    Utils::Text::Position positionFromIndex(const QModelIndex &sourceIndex) const;
    using Range = std::pair<Utils::Text::Position, Utils::Text::Position>;
    Range rangeFromIndex(const QModelIndex &sourceIndex) const;

    // line is 1-based and column is 0-based
    QModelIndex indexForPosition(int line, int column, const QModelIndex &rootIndex = {}) const;

private:
    void rebuild();
    CPlusPlus::Symbol *symbolFromIndex(const QModelIndex &index) const;
    int globalSymbolCount() const;
    CPlusPlus::Symbol *globalSymbolAt(int index) const;
    void buildTree(SymbolItem *root, bool isRoot);

private:
    CPlusPlus::Document::Ptr m_candidate;
    CPlusPlus::Document::Ptr m_cppDocument;
    CPlusPlus::Overview m_overview;
    friend class SymbolItem;
    QTimer *m_updateTimer = nullptr;
};

} // namespace CppEditor::Internal
