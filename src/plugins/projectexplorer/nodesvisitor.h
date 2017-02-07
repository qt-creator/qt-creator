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

#include "projectexplorer_export.h"

#include <utils/fileutils.h>

#include <QStringList>

namespace ProjectExplorer {

class Node;
class FileNode;
class SessionNode;
class FolderNode;

class PROJECTEXPLORER_EXPORT NodesVisitor
{
public:
    virtual ~NodesVisitor();

    virtual void visitFolderNode(FolderNode *) { }

protected:
     NodesVisitor() {}
};

/* useful visitors */

class PROJECTEXPLORER_EXPORT FindNodesForFileVisitor : public NodesVisitor
{
public:
    explicit FindNodesForFileVisitor(const Utils::FileName &fileToSearch);

    QList<Node*> nodes() const;

    void visitFolderNode(FolderNode *node) override;

private:
    Utils::FileName m_path;
    QList<Node *> m_nodes;
};

class PROJECTEXPLORER_EXPORT FindAllFilesVisitor : public NodesVisitor
{
public:
    Utils::FileNameList filePaths() const;

    void visitFolderNode(FolderNode *folderNode) override;

private:
    Utils::FileNameList m_filePaths;
};

} // namespace ProjectExplorer
