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

#include <QObject>

#include "classviewparsertreeitem.h"

#include <cplusplus/CPlusPlusForwardDeclarations.h>
#include <cplusplus/CppDocument.h>

// might be changed to forward declaration - is not done to be less dependent
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/project.h>

#include <QList>
#include <QSharedPointer>
#include <QStandardItem>

namespace ClassView {
namespace Internal {

class ParserPrivate;

class Parser : public QObject
{
    Q_OBJECT

public:
    explicit Parser(QObject *parent = 0);
    ~Parser();

    bool canFetchMore(QStandardItem *item, bool skipRoot = false) const;

    void fetchMore(QStandardItem *item, bool skipRoot = false) const;
    bool hasChildren(QStandardItem *item) const;

signals:
    //! File list is changed
    void filesAreRemoved();

    void treeDataUpdate(QSharedPointer<QStandardItem> result);

    void resetDataDone();

public:
    void clearCacheAll();

    void clearCache();

    void requestCurrentState();

    void setFileList(const QStringList &fileList);

    void removeFiles(const QStringList &fileList);

    void resetData(const CPlusPlus::Snapshot &snapshot);

    void resetDataToCurrentState();

    void parseDocument(const CPlusPlus::Document::Ptr &doc);

    void setFlatMode(bool flat);

protected:
    typedef QHash<QString, unsigned>::const_iterator CitCachedDocTreeRevision;
    typedef QHash<QString, QStringList>::const_iterator CitCachedPrjFileLists;

    void onResetDataDone();

    void addProject(const ParserTreeItem::Ptr &item, const QStringList &fileList,
                    const QString &projectId = QString());

    void addSymbol(const ParserTreeItem::Ptr &item, const CPlusPlus::Symbol *symbol);

    ParserTreeItem::ConstPtr getParseDocumentTree(const CPlusPlus::Document::Ptr &doc);

    ParserTreeItem::ConstPtr getCachedOrParseDocumentTree(const CPlusPlus::Document::Ptr &doc);

    ParserTreeItem::Ptr getParseProjectTree(const QStringList &fileList, const QString &projectId);

    ParserTreeItem::Ptr getCachedOrParseProjectTree(const QStringList &fileList,
                                                    const QString &projectId);

    void emitCurrentTree();

    ParserTreeItem::ConstPtr parse();

    ParserTreeItem::ConstPtr findItemByRoot(const QStandardItem *item, bool skipRoot = false) const;

    QStringList addProjectNode(const ParserTreeItem::Ptr &item,
                               const ProjectExplorer::ProjectNode *node);
    QStringList getAllFiles(const ProjectExplorer::ProjectNode *node);
    void addFlatTree(const ParserTreeItem::Ptr &item,
                               const ProjectExplorer::ProjectNode *node);

    QStringList projectNodeFileList(const ProjectExplorer::FolderNode *node) const;

private:
    //! Private class data pointer
    ParserPrivate *d;
};

} // namespace Internal
} // namespace ClassView
