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

#include <projectexplorer/projectnodes.h>

namespace CMakeProjectManager {
namespace Internal {

class CMakeInputsNode : public ProjectExplorer::ProjectNode
{
public:
    CMakeInputsNode(const Utils::FileName &cmakeLists);

    static QByteArray generateId(const Utils::FileName &inputFile);

    bool showInSimpleTree() const final;
};

class CMakeListsNode : public ProjectExplorer::ProjectNode
{
public:
    CMakeListsNode(const Utils::FileName &cmakeListPath);

    bool showInSimpleTree() const final;
    bool supportsAction(ProjectExplorer::ProjectAction action, const Node *node) const override;
    virtual Utils::optional<Utils::FileName> visibleAfterAddFileAction() const override;
};

class CMakeProjectNode : public ProjectExplorer::ProjectNode
{
public:
    CMakeProjectNode(const Utils::FileName &directory);

    bool showInSimpleTree() const final;
    QString tooltip() const final;

    bool addFiles(const QStringList &filePaths, QStringList *notAdded) override;
};

class CMakeTargetNode : public ProjectExplorer::ProjectNode
{
public:
    CMakeTargetNode(const Utils::FileName &directory, const QString &target);

    static QByteArray generateId(const Utils::FileName &directory, const QString &target);

    void setTargetInformation(const QList<Utils::FileName> &artifacts, const QString &type);

    bool showInSimpleTree() const final;
    QString tooltip() const final;

    bool supportsAction(ProjectExplorer::ProjectAction action, const Node *node) const override;
    bool addFiles(const QStringList &filePaths, QStringList *notAdded) override;
    virtual Utils::optional<Utils::FileName> visibleAfterAddFileAction() const override;

private:
    QString m_tooltip;
};

} // namespace Internal
} // namespace CMakeProjectManager
