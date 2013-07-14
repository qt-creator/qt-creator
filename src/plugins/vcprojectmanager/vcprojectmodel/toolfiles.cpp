/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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

namespace VcProjectManager {
namespace Internal {

ToolFiles::ToolFiles()
{
}

ToolFiles::ToolFiles(const ToolFiles &toolFiles)
{
    foreach (const ToolFile::Ptr &toolFile, toolFiles.m_toolFiles)
        m_toolFiles.append(ToolFile::Ptr(new ToolFile(*toolFile)));

    foreach (const DefaultToolFile::Ptr &toolFile, toolFiles.m_defaultToolFiles)
        m_defaultToolFiles.append(DefaultToolFile::Ptr(new DefaultToolFile(*toolFile)));
}

ToolFiles &ToolFiles::operator =(const ToolFiles &toolFiles)
{
    if (this != &toolFiles) {
        m_toolFiles.clear();
        m_defaultToolFiles.clear();

        foreach (const ToolFile::Ptr &toolFile, toolFiles.m_toolFiles)
            m_toolFiles.append(ToolFile::Ptr(new ToolFile(*toolFile)));

        foreach (const DefaultToolFile::Ptr &toolFile, toolFiles.m_defaultToolFiles)
            m_defaultToolFiles.append(DefaultToolFile::Ptr(new DefaultToolFile(*toolFile)));
    }

    return *this;
}

ToolFiles::~ToolFiles()
{
    m_defaultToolFiles.clear();
    m_toolFiles.clear();
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

void ToolFiles::processNodeAttributes(const QDomElement &element)
{
    Q_UNUSED(element)
}

VcNodeWidget *ToolFiles::createSettingsWidget()
{
    return 0;
}

QDomNode ToolFiles::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement toolFilesElement = domXMLDocument.createElement(QLatin1String("ToolFiles"));

    foreach (const ToolFile::Ptr &file, m_toolFiles)
        toolFilesElement.appendChild(file->toXMLDomNode(domXMLDocument));

    foreach (const DefaultToolFile::Ptr &file, m_defaultToolFiles)
        toolFilesElement.appendChild(file->toXMLDomNode(domXMLDocument));

    return toolFilesElement;
}

bool ToolFiles::isEmpty() const
{
    return m_defaultToolFiles.isEmpty() && m_toolFiles.isEmpty();
}

void ToolFiles::addToolFile(ToolFile::Ptr toolFile)
{
    if (m_toolFiles.contains(toolFile))
        return;

    foreach (const ToolFile::Ptr &toolF, m_toolFiles) {
        if (toolF->relativePath() == toolFile->relativePath())
            return;
    }
    m_toolFiles.append(toolFile);
}

void ToolFiles::removeToolFile(ToolFile::Ptr toolFile)
{
    m_toolFiles.removeAll(toolFile);
}

void ToolFiles::removeToolFile(const QString &relativeToolFilePath)
{
    foreach (const ToolFile::Ptr &toolF, m_toolFiles) {
        if (toolF->relativePath() == relativeToolFilePath) {
            removeToolFile(toolF);
            return;
        }
    }
}

QList<ToolFile::Ptr > ToolFiles::toolFiles() const
{
    return m_toolFiles;
}

ToolFile::Ptr ToolFiles::toolFile(const QString &relativePath)
{
    foreach (const ToolFile::Ptr &toolFile, m_toolFiles) {
        if (toolFile->relativePath() == relativePath)
            return toolFile;
    }
    return ToolFile::Ptr();
}

void ToolFiles::addDefaultToolFile(DefaultToolFile::Ptr defToolFile)
{
    if (m_defaultToolFiles.contains(defToolFile))
        return;

    foreach (const DefaultToolFile::Ptr &toolF, m_defaultToolFiles) {
        if (toolF->fileName() == defToolFile->fileName())
            return;
    }
    m_defaultToolFiles.append(defToolFile);
}

void ToolFiles::removeDefaultToolFile(DefaultToolFile::Ptr defToolFile)
{
    m_defaultToolFiles.removeAll(defToolFile);
}

void ToolFiles::removeDefaultToolFile(const QString &fileName)
{
    foreach (const DefaultToolFile::Ptr &toolF, m_defaultToolFiles) {
        if (toolF->fileName() == fileName) {
            removeDefaultToolFile(toolF);
            return;
        }
    }
}

QList<DefaultToolFile::Ptr > ToolFiles::defaultToolFiles() const
{
    return m_defaultToolFiles;
}

DefaultToolFile::Ptr ToolFiles::defaultToolFile(const QString &fileName)
{
    foreach (DefaultToolFile::Ptr defToolFile, m_defaultToolFiles)
        if (defToolFile->fileName() == fileName)
            return defToolFile;
    return DefaultToolFile::Ptr();
}

void ToolFiles::processToolFiles(const QDomNode &toolFileNode)
{
    if (toolFileNode.nodeName() == QLatin1String("ToolFile")) {
        ToolFile::Ptr toolFile(new ToolFile);
        m_toolFiles.append(toolFile);
        toolFile->processNode(toolFileNode);
    }

    else if (toolFileNode.nodeName() == QLatin1String("DefaultToolFile")) {
        DefaultToolFile::Ptr defToolFile(new DefaultToolFile);
        m_defaultToolFiles.append(defToolFile);
        defToolFile->processNode(toolFileNode);
    }

    // process next sibling
    QDomNode nextSibling = toolFileNode.nextSibling();
    if (!nextSibling.isNull())
        processToolFiles(nextSibling);
}

} // namespace Internal
} // namespace VcProjectManager
