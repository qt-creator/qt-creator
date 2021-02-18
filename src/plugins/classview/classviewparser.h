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

    void requestCurrentState();
    void removeFiles(const QStringList &fileList);
    void resetDataToCurrentState();
    void setFlatMode(bool flat);

    void updateDocuments(const QList<CPlusPlus::Document::Ptr> &docs);

signals:
    void treeRegenerated(const ParserTreeItem::ConstPtr &root);

private:
    void setFileList(const QStringList &fileList);
    void resetData(const CPlusPlus::Snapshot &snapshot);

    ParserTreeItem::ConstPtr getParseDocumentTree(const CPlusPlus::Document::Ptr &doc);
    ParserTreeItem::ConstPtr getCachedOrParseDocumentTree(const CPlusPlus::Document::Ptr &doc);
    ParserTreeItem::ConstPtr getParseProjectTree(const QStringList &fileList, const QString &projectId);
    ParserTreeItem::ConstPtr getCachedOrParseProjectTree(const QStringList &fileList,
                                                    const QString &projectId);
    ParserTreeItem::ConstPtr parse();

    QStringList getAllFiles(const ProjectExplorer::Project *project);
    ParserTreeItem::ConstPtr addFlatTree(const ProjectExplorer::Project *project);

    //! Private class data pointer
    ParserPrivate *d;
};

} // namespace Internal
} // namespace ClassView
