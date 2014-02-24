/**************************************************************************
**
** Copyright (c) 2014 Bojan Petrovic
** Copyright (c) 2014 Radovan Zivkovic
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
#include "toolfiles.h"
#include "../interfaces/iattributecontainer.h"
#include "vcprojectdocument_constants.h"

#include <utils/qtcassert.h>

namespace VcProjectManager {
namespace Internal {

ToolFiles::ToolFiles()
{
}

ToolFiles::ToolFiles(const ToolFiles &toolFiles)
{
    foreach (const IToolFile *toolFile, toolFiles.m_toolFiles)
        m_toolFiles.append(toolFile->clone());
}

ToolFiles &ToolFiles::operator =(const ToolFiles &toolFiles)
{
    if (this != &toolFiles) {
        qDeleteAll(m_toolFiles);
        m_toolFiles.clear();

        foreach (const IToolFile *toolFile, toolFiles.m_toolFiles)
            m_toolFiles.append(toolFile->clone());
    }

    return *this;
}

ToolFiles::~ToolFiles()
{
    qDeleteAll(m_toolFiles);
}

void ToolFiles::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.hasChildNodes()) {
        QDomNode firstChild = node.firstChild();

        if (!firstChild.isNull())
            processToolFiles(firstChild);
    }
}

VcNodeWidget *ToolFiles::createSettingsWidget()
{
    return 0;
}

QDomNode ToolFiles::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement toolFilesElement = domXMLDocument.createElement(QLatin1String("ToolFiles"));

    foreach (const IToolFile *toolFile, m_toolFiles)
        toolFilesElement.appendChild(toolFile->toXMLDomNode(domXMLDocument));

    return toolFilesElement;
}

void ToolFiles::addToolFile(IToolFile *toolFile)
{
    if (!toolFile || m_toolFiles.contains(toolFile))
        return;
    m_toolFiles.append(toolFile);
}

int ToolFiles::toolFileCount() const
{
    return m_toolFiles.size();
}

IToolFile *ToolFiles::toolFile(int index) const
{
    QTC_ASSERT(0 <= index && index < m_toolFiles.size(), return 0);
    return m_toolFiles[index];
}

void ToolFiles::removeToolFile(IToolFile *toolFile)
{
    if (m_toolFiles.removeOne(toolFile))
        delete toolFile;
}

void ToolFiles::processToolFiles(const QDomNode &toolFileNode)
{
    IToolFile *toolFile = 0;
    if (toolFileNode.nodeName() == QLatin1String(VcDocConstants::TOOL_FILE))
        toolFile = new ToolFile;

    else if (toolFileNode.nodeName() == QLatin1String(VcDocConstants::DEFAULT_TOOL_FILE))
        toolFile = new DefaultToolFile;

    if (toolFile) {
        toolFile->processNode(toolFileNode);
        m_toolFiles.append(toolFile);
    }
    // process next sibling
    QDomNode nextSibling = toolFileNode.nextSibling();
    if (!nextSibling.isNull())
        processToolFiles(nextSibling);
}

} // namespace Internal
} // namespace VcProjectManager
