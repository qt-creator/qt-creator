/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/projectnodes.h>
#include <utils/fileutils.h>

namespace MesonProjectManager {
namespace Internal {
class MesonProjectNode : public ProjectExplorer::ProjectNode
{
public:
    MesonProjectNode(const Utils::FilePath &directory);
};

class MesonTargetNode : public ProjectExplorer::ProjectNode
{
public:
    MesonTargetNode(const Utils::FilePath &directory, const QString &name);
    void build() override;
    QString tooltip() const final;
    QString buildKey() const final;

private:
    QString m_name;
};

class MesonFileNode : public ProjectExplorer::ProjectNode
{
public:
    MesonFileNode(const Utils::FilePath &file);
    bool showInSimpleTree() const final { return false; }
    Utils::optional<Utils::FilePath> visibleAfterAddFileAction() const override
    {
        return filePath().pathAppended("meson.build");
    }
};
} // namespace Internal
} // namespace MesonProjectManager
