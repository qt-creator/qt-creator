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
    explicit Parser(QObject *parent = nullptr);
    ~Parser() override;

    // TODO: below three methods are called directly from different thread
    bool canFetchMore(QStandardItem *item, bool skipRoot = false) const;
    void fetchMore(QStandardItem *item, bool skipRoot = false) const;
    bool hasChildren(QStandardItem *item) const;

    void clearCache();
    void requestCurrentState();
    void removeFiles(const QStringList &fileList);
    void resetDataToCurrentState();
    void parseDocument(const CPlusPlus::Document::Ptr &doc);
    void setFlatMode(bool flat);

signals:
    void treeDataUpdate(QSharedPointer<QStandardItem> result);

private:
    using CitCachedDocTreeRevision = QHash<QString, unsigned>::const_iterator;
    using CitCachedPrjFileLists = QHash<QString, QStringList>::const_iterator;

    void setFileList(const QStringList &fileList);
    void resetData(const CPlusPlus::Snapshot &snapshot);
    void addProject(const ParserTreeItem::Ptr &item, const QStringList &fileList,
                    const QString &projectId = QString());

    void addSymbol(const ParserTreeItem::Ptr &item, const CPlusPlus::Symbol *symbol);

    ParserTreeItem::ConstPtr getParseDocumentTree(const CPlusPlus::Document::Ptr &doc);
    ParserTreeItem::ConstPtr getCachedOrParseDocumentTree(const CPlusPlus::Document::Ptr &doc);
    ParserTreeItem::Ptr getParseProjectTree(const QStringList &fileList, const QString &projectId);
    ParserTreeItem::Ptr getCachedOrParseProjectTree(const QStringList &fileList,
                                                    const QString &projectId);
    ParserTreeItem::ConstPtr parse();
    ParserTreeItem::ConstPtr findItemByRoot(const QStandardItem *item, bool skipRoot = false) const;

    QStringList addProjectTree(const ParserTreeItem::Ptr &item, const ProjectExplorer::Project *project);
    QStringList getAllFiles(const ProjectExplorer::Project *project);
    void addFlatTree(const ParserTreeItem::Ptr &item, const ProjectExplorer::Project *project);

    //! Private class data pointer
    ParserPrivate *d;
};

} // namespace Internal
} // namespace ClassView
