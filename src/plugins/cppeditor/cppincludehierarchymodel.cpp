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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppincludehierarchymodel.h"

#include "cppincludehierarchyitem.h"

#include <coreplugin/fileiconprovider.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cpptoolseditorsupport.h>
#include <texteditor/basetexteditor.h>

#include <cplusplus/CppDocument.h>

#include <QSet>

using namespace CPlusPlus;
using namespace CppTools;

namespace CppEditor {
namespace Internal {

CppIncludeHierarchyModel::CppIncludeHierarchyModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootItem(new CppIncludeHierarchyItem(QString()))
    , m_includesItem(new CppIncludeHierarchyItem(tr("Includes"), m_rootItem))
    , m_includedByItem(new CppIncludeHierarchyItem(tr("Included by"), m_rootItem))
    , m_editor(0)
{
    m_rootItem->appendChild(m_includesItem);
    m_rootItem->appendChild(m_includedByItem);
}

CppIncludeHierarchyModel::~CppIncludeHierarchyModel()
{
    delete m_rootItem;
}

QModelIndex CppIncludeHierarchyModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
          return QModelIndex();

      CppIncludeHierarchyItem *parentItem = parent.isValid()
              ? static_cast<CppIncludeHierarchyItem*>(parent.internalPointer())
              : m_rootItem;

      CppIncludeHierarchyItem *childItem = parentItem->child(row);
      return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex CppIncludeHierarchyModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    CppIncludeHierarchyItem *childItem
            = static_cast<CppIncludeHierarchyItem*>(index.internalPointer());
    CppIncludeHierarchyItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int CppIncludeHierarchyModel::rowCount(const QModelIndex &parent) const
{
    CppIncludeHierarchyItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<CppIncludeHierarchyItem*>(parent.internalPointer());

    return parentItem->childCount();
}

int CppIncludeHierarchyModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant CppIncludeHierarchyModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    CppIncludeHierarchyItem *item = static_cast<CppIncludeHierarchyItem*>(index.internalPointer());

    if (!item)
        return QVariant();

    if (role == Qt::DisplayRole) {
        if ((item == m_includesItem && m_includesItem->childCount() == 0)
                || (item == m_includedByItem && m_includedByItem->childCount() == 0)) {
            return QString(item->fileName() + QLatin1Char(' ') + tr("(none)"));
        }

        if (item->isCyclic())
            return QString(item->fileName() + QLatin1Char(' ') +  tr("(cyclic)"));

        return item->fileName();
    }

    if (item == m_rootItem || item == m_includesItem || item == m_includedByItem)
        return QVariant();

    switch (role) {
    case Qt::ToolTipRole:
        return item->filePath();
    case Qt::DecorationRole:
        return Core::FileIconProvider::icon(QFileInfo(item->filePath()));
    case LinkRole: {
        QVariant itemLink;
        TextEditor::BaseTextEditorWidget::Link link(item->filePath(), item->line());
        itemLink.setValue(link);
        return itemLink;
    }
    }

    return QVariant();
}

void CppIncludeHierarchyModel::fetchMore(const QModelIndex &parent)
{
    if (!parent.isValid())
        return;

    CppIncludeHierarchyItem *parentItem
            = static_cast<CppIncludeHierarchyItem*>(parent.internalPointer());
    Q_ASSERT(parentItem);

    if (parentItem == m_rootItem || parentItem == m_includesItem || parentItem == m_includedByItem)
        return;

    if (parentItem->needChildrenPopulate()) {
        QSet<QString> cyclic;
        cyclic << m_editor->document()->filePath();
        CppIncludeHierarchyItem *item = parentItem->parent();
        while (!(item == m_includesItem || item == m_includedByItem)) {
            cyclic << item->filePath();
            item = item->parent();
        }

        if (item == m_includesItem)
            buildHierarchyIncludes_helper(parentItem->filePath(), parentItem, &cyclic);
        else
            buildHierarchyIncludedBy_helper(parentItem->filePath(), parentItem, &cyclic);
    }

}

bool CppIncludeHierarchyModel::canFetchMore(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return false;

    CppIncludeHierarchyItem *parentItem
            = static_cast<CppIncludeHierarchyItem*>(parent.internalPointer());
    Q_ASSERT(parentItem);

    if (parentItem == m_includesItem || parentItem == m_includedByItem)
        return false;

    if (parentItem->needChildrenPopulate())
        return true;

    return false;
}

void CppIncludeHierarchyModel::clear()
{
    beginResetModel();
    m_includesItem->removeChildren();
    m_includedByItem->removeChildren();
    endResetModel();
}

void CppIncludeHierarchyModel::buildHierarchy(TextEditor::BaseTextEditor *editor,
                                              const QString &filePath)
{
    m_editor = editor;
    beginResetModel();
    buildHierarchyIncludes(filePath);
    buildHierarchyIncludedBy(filePath);
    endResetModel();
}

bool CppIncludeHierarchyModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return true;
    CppIncludeHierarchyItem *parentItem
            = static_cast<CppIncludeHierarchyItem*>(parent.internalPointer());

    Q_ASSERT(parentItem);
    return parentItem->hasChildren();
}

bool CppIncludeHierarchyModel::isEmpty() const
{
    return !m_includesItem->hasChildren() && !m_includedByItem->hasChildren();
}

void CppIncludeHierarchyModel::buildHierarchyIncludes(const QString &currentFilePath)
{
    QSet<QString> cyclic;
    buildHierarchyIncludes_helper(currentFilePath, m_includesItem, &cyclic);
}

void CppIncludeHierarchyModel::buildHierarchyIncludes_helper(const QString &filePath,
                                                             CppIncludeHierarchyItem *parent,
                                                             QSet<QString> *cyclic, bool recursive)
{
    if (!m_editor)
        return;

    CppModelManagerInterface *cppMM = CppModelManagerInterface::instance();
    const Snapshot &snapshot = cppMM->cppEditorSupport(m_editor)->snapshotUpdater()->snapshot();
    Document::Ptr doc = snapshot.document(filePath);
    if (!doc)
        return;

    parent->setHasChildren(doc->resolvedIncludes().size() > 0);
    if (!recursive)
        return;

    cyclic->insert(filePath);

    foreach (const Document::Include &includeFile, doc->resolvedIncludes()) {
        const QString includedFilePath = includeFile.resolvedFileName();
        CppIncludeHierarchyItem *item = 0;

        if (cyclic->contains(includedFilePath)) {
            item = new CppIncludeHierarchyItem(includedFilePath, parent, true);
            parent->appendChild(item);
            continue;
        }
        item = new CppIncludeHierarchyItem(includedFilePath, parent);
        parent->appendChild(item);
        buildHierarchyIncludes_helper(includedFilePath, item, cyclic, false);

    }
    cyclic->remove(filePath);
}

void CppIncludeHierarchyModel::buildHierarchyIncludedBy(const QString &currentFilePath)
{
    QSet<QString> cyclic;
    buildHierarchyIncludedBy_helper(currentFilePath, m_includedByItem, &cyclic);
}

void CppIncludeHierarchyModel::buildHierarchyIncludedBy_helper(const QString &filePath,
                                                               CppIncludeHierarchyItem *parent,
                                                               QSet<QString> *cyclic,
                                                               bool recursive)
{
    cyclic->insert(filePath);
    const Snapshot &snapshot = CppTools::CppModelManagerInterface::instance()->snapshot();
    Snapshot::const_iterator citEnd = snapshot.end();
    for (Snapshot::const_iterator cit = snapshot.begin(); cit != citEnd; ++cit) {
        const QString filePathFromSnapshot = cit.key();
        Document::Ptr doc = cit.value();
        foreach (const Document::Include &includeFile, doc->resolvedIncludes()) {
            const QString includedFilePath = includeFile.resolvedFileName();

            if (includedFilePath == filePath) {
                parent->setHasChildren(true);
                if (!recursive) {
                    cyclic->remove(filePath);
                    return;
                }

                const bool isCyclic = cyclic->contains(filePathFromSnapshot);
                CppIncludeHierarchyItem *item = new CppIncludeHierarchyItem(filePathFromSnapshot,
                                                                            parent,
                                                                            isCyclic);
                item->setLine(includeFile.line());
                parent->appendChild(item);

                if (isCyclic)
                    continue;
                else
                    buildHierarchyIncludedBy_helper(filePathFromSnapshot, item, cyclic, false);
            }
        }
    }
    cyclic->remove(filePath);
}

} // namespace Internal
} // namespace CppEditor
