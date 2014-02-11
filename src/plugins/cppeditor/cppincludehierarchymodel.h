/****************************************************************************
**
** Copyright (C) 2014 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>
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

#ifndef CPPINCLUDEHIERARCHYMODEL_H
#define CPPINCLUDEHIERARCHYMODEL_H

#include <QAbstractItemModel>

namespace {

enum ItemRole {
    AnnotationRole = Qt::UserRole + 1,
    LinkRole
};

} // Anonymous

namespace Core { class IEditor; }

namespace CppEditor {
namespace Internal {

class CPPEditor;
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

    void clear();
    void buildHierarchy(CPPEditor *editor, const QString &filePath);
    bool isEmpty() const;

private:
    void buildHierarchyIncludes(const QString &currentFilePath);
    void buildHierarchyIncludes_helper(const QString &filePath, CppIncludeHierarchyItem *parent,
                                       QSet<QString> *cyclic, bool recursive = true);
    void buildHierarchyIncludedBy(const QString &currentFilePath);
    void buildHierarchyIncludedBy_helper(const QString &filePath, CppIncludeHierarchyItem *parent,
                                         QSet<QString> *cyclic, bool recursive = true);

    CppIncludeHierarchyItem *m_rootItem;
    CppIncludeHierarchyItem *m_includesItem;
    CppIncludeHierarchyItem *m_includedByItem;
    CPPEditor *m_editor;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPINCLUDEHIERARCHYMODEL_H
