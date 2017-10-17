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

#include "cmakeprojectnodes.h"

#include "cmakeprojectconstants.h"

#include <coreplugin/fileiconprovider.h>

#include <utils/algorithm.h>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

CMakeInputsNode::CMakeInputsNode(const Utils::FileName &cmakeLists) :
    ProjectExplorer::ProjectNode(cmakeLists, generateId(cmakeLists))
{
    setPriority(Node::DefaultPriority - 10); // Bottom most!
    setDisplayName(QCoreApplication::translate("CMakeFilesProjectNode", "CMake Modules"));
    setIcon(QIcon(":/projectexplorer/images/session.png")); // TODO: Use a better icon!
}

QByteArray CMakeInputsNode::generateId(const Utils::FileName &inputFile)
{
    return inputFile.toString().toUtf8() + "/cmakeInputs";
}

bool CMakeInputsNode::showInSimpleTree() const
{
    return true;
}

CMakeListsNode::CMakeListsNode(const Utils::FileName &cmakeListPath) :
    ProjectExplorer::ProjectNode(cmakeListPath)
{
    static QIcon folderIcon = Core::FileIconProvider::directoryIcon(Constants::FILEOVERLAY_CMAKE);
    setIcon(folderIcon);
}

bool CMakeListsNode::showInSimpleTree() const
{
    return false;
}

CMakeProjectNode::CMakeProjectNode(const Utils::FileName &directory) :
    ProjectExplorer::ProjectNode(directory)
{
    setPriority(Node::DefaultProjectPriority + 1000);
    setIcon(QIcon(":/projectexplorer/images/projectexplorer.png")); // TODO: Use proper icon!
}

bool CMakeProjectNode::showInSimpleTree() const
{
    return true;
}

QString CMakeProjectNode::tooltip() const
{
    return QString();
}

CMakeTargetNode::CMakeTargetNode(const Utils::FileName &directory, const QString &target) :
    ProjectExplorer::ProjectNode(directory, generateId(directory, target))
{
    setPriority(Node::DefaultProjectPriority + 900);
    setIcon(QIcon(":/projectexplorer/images/build.png")); // TODO: Use proper icon!
}

QByteArray CMakeTargetNode::generateId(const Utils::FileName &directory, const QString &target)
{
    return directory.toString().toUtf8() + "///::///" + target.toUtf8();
}

bool CMakeTargetNode::showInSimpleTree() const
{
    return true;
}

QString CMakeTargetNode::tooltip() const
{
    return m_tooltip;
}

void CMakeTargetNode::setTargetInformation(const QList<Utils::FileName> &artifacts,
                                           const QString &type)
{
    m_tooltip = QCoreApplication::translate("CMakeTargetNode", "Target type: ") + type + "<br>";
    if (artifacts.isEmpty()) {
        m_tooltip += QCoreApplication::translate("CMakeTargetNode", "No build artifacts");
    } else {
        const QStringList tmp = Utils::transform(artifacts, &Utils::FileName::toUserOutput);
        m_tooltip += QCoreApplication::translate("CMakeTargetNode", "Build artifacts:<br>")
                + tmp.join("<br>");
    }
}
