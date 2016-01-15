/****************************************************************************
**
** Copyright (C) 2016 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>
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

#ifndef CPPINCLUDEHIERARCHYMODEL_H
#define CPPINCLUDEHIERARCHYMODEL_H

#include <QAbstractItemModel>

namespace {

enum ItemRole {
    AnnotationRole = Qt::UserRole + 1,
    LinkRole
};

} // Anonymous

namespace CPlusPlus { class Snapshot; }
namespace TextEditor { class BaseTextEditor; }

namespace CppEditor {
namespace Internal {

class CppEditor;
class CppIncludeHierarchyItem;

class CppIncludeHierarchyModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit CppIncludeHierarchyModel(QObject *parent);
    ~CppIncludeHierarchyModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    void fetchMore(const QModelIndex &parent);
    bool canFetchMore(const QModelIndex &parent) const;
    bool hasChildren(const QModelIndex &parent) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    Qt::DropActions supportedDragActions() const;
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;

    void clear();
    void buildHierarchy(TextEditor::BaseTextEditor *editor, const QString &filePath);
    bool isEmpty() const;

private:
    void buildHierarchyIncludes(const QString &currentFilePath);
    void buildHierarchyIncludes_helper(const QString &filePath, CppIncludeHierarchyItem *parent,
                                       CPlusPlus::Snapshot snapshot,
                                       QSet<QString> *cyclic, bool recursive = true);
    void buildHierarchyIncludedBy(const QString &currentFilePath);
    void buildHierarchyIncludedBy_helper(const QString &filePath, CppIncludeHierarchyItem *parent,
                                         CPlusPlus::Snapshot snapshot, QSet<QString> *cyclic,
                                         bool recursive = true);

    CppIncludeHierarchyItem *m_rootItem;
    CppIncludeHierarchyItem *m_includesItem;
    CppIncludeHierarchyItem *m_includedByItem;
    TextEditor::BaseTextEditor *m_editor;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPINCLUDEHIERARCHYMODEL_H
