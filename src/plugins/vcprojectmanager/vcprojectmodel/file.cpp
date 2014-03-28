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
#include "configurationcontainer.h"
#include "file.h"
#include "filebuildconfiguration.h"
#include "generalattributecontainer.h"
#include "vcprojectdocument.h"
#include "../interfaces/iattributedescriptiondataitem.h"
#include "../interfaces/iconfigurationbuildtool.h"
#include "../interfaces/iconfigurationbuildtools.h"
#include "../interfaces/isectioncontainer.h"
#include "../interfaces/itoolattribute.h"
#include "../interfaces/itoolattributecontainer.h"
#include "../interfaces/itooldescription.h"
#include "../interfaces/itools.h"
#include "../interfaces/itoolsection.h"
#include "../interfaces/itoolsectiondescription.h"
#include "../widgets/fileconfigurationseditwidget.h"
#include "tools/tool_constants.h"
#include "tools/toolattributes/tooldescriptiondatamanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QDomNode>

namespace VcProjectManager {
namespace Internal {

File::File(IVisualStudioProject *parentProjectDoc)
    : m_parentProjectDoc(parentProjectDoc)
{
    m_configurationContainer = new ConfigurationContainer;
    m_attributeContainer = new GeneralAttributeContainer;
}

File::File(const File &file)
{
    m_attributeContainer = new GeneralAttributeContainer;
    m_configurationContainer = new ConfigurationContainer;

    m_parentProjectDoc = file.m_parentProjectDoc;
    m_relativePath = file.m_relativePath;
    *m_configurationContainer = *(file.m_configurationContainer);
    *m_attributeContainer = *(file.m_attributeContainer);

    foreach (const File *f, file.m_files)
        m_files.append(new File(*f));
}

File &File::operator =(const File &file)
{
    if (this != &file) {
        m_parentProjectDoc = file.m_parentProjectDoc;
        m_relativePath = file.m_relativePath;
        *m_configurationContainer = *(file.m_configurationContainer);
        *m_attributeContainer = *(file.m_attributeContainer);

        qDeleteAll(m_files);

        foreach (const File *f, file.m_files)
            m_files.append(new File(*f));
    }
    return *this;
}

File::~File()
{
    qDeleteAll(m_files);
    delete m_configurationContainer;
    delete m_attributeContainer;
}

void File::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeType() == QDomNode::ElementNode)
        processNodeAttributes(node.toElement());

    copyProjectConfigs();

    if (node.hasChildNodes()) {
        QDomNode firstChild = node.firstChild();
        if (!firstChild.isNull()) {
            if (firstChild.nodeName() == QLatin1String("FileConfiguration"))
                processFileConfiguration(firstChild);
            else if (firstChild.nodeName() == QLatin1String("File"))
                processFile(firstChild);
        }
    }
}

VcNodeWidget *File::createSettingsWidget()
{
    return new FileConfigurationsEditWidget(this, m_parentProjectDoc);
}

QDomNode File::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement fileNode = domXMLDocument.createElement(QLatin1String("File"));

    fileNode.setAttribute(QLatin1String("RelativePath"), m_relativePath);

    foreach (const File *file, m_files)
        fileNode.appendChild(file->toXMLDomNode(domXMLDocument));

    m_configurationContainer->appendToXMLNode(fileNode, domXMLDocument);
    m_attributeContainer->appendToXMLNode(fileNode);
    return fileNode;
}

ConfigurationContainer *File::configurationContainer() const
{
    return m_configurationContainer;
}

IAttributeContainer *File::attributeContainer() const
{
    return m_attributeContainer;
}

QString File::relativePath() const
{
    return m_relativePath;
}

void File::setRelativePath(const QString &relativePath)
{
    m_relativePath = relativePath;
}

IFile *File::clone() const
{
    return new File(*this);
}

ProjectExplorer::FileType File::fileType() const
{
    QString mimeType = Core::MimeDatabase::findByFile(canonicalPath()).type();

    if (mimeType == QLatin1String(ProjectExplorer::Constants::CPP_SOURCE_MIMETYPE)
            || mimeType == QLatin1String(ProjectExplorer::Constants::C_SOURCE_MIMETYPE))
        return ProjectExplorer::SourceType;
    if (mimeType == QLatin1String(ProjectExplorer::Constants::CPP_HEADER_MIMETYPE)
            || mimeType == QLatin1String(ProjectExplorer::Constants::C_HEADER_MIMETYPE))
        return ProjectExplorer::HeaderType;
    if (mimeType == QLatin1String(ProjectExplorer::Constants::RESOURCE_MIMETYPE))
        return ProjectExplorer::ResourceType;
    if (mimeType == QLatin1String(ProjectExplorer::Constants::FORM_MIMETYPE))
        return ProjectExplorer::FormType;
    if (mimeType == QLatin1String(ProjectExplorer::Constants::QML_MIMETYPE))
        return ProjectExplorer::QMLType;

    return ProjectExplorer::UnknownFileType;
}

QString File::canonicalPath() const
{
    if (m_parentProjectDoc) {
        QFileInfo fileInfo(m_parentProjectDoc->filePath());
        fileInfo = QFileInfo(fileInfo.canonicalPath() + QLatin1Char('/') + m_relativePath);
        return fileInfo.canonicalFilePath();
    }

    return QString() + m_relativePath;
}

void File::processFileConfiguration(const QDomNode &fileConfigNode)
{
    IConfiguration *fileConfig = new FileBuildConfiguration(m_parentProjectDoc);
    fileConfig->processNode(fileConfigNode);

    IConfiguration *projConfig = m_configurationContainer->configuration(fileConfig->fullName());

    if (projConfig) {
        copyAllNonDefaultToolAtributes(fileConfig, projConfig);

        m_configurationContainer->removeConfiguration(projConfig->fullName());
        m_configurationContainer->addConfiguration(fileConfig);

        // process next sibling
        QDomNode nextSibling = fileConfigNode.nextSibling();
        if (!nextSibling.isNull()) {
            if (nextSibling.nodeName() == QLatin1String("FileConfiguration"))
                processFileConfiguration(nextSibling);
            else
                processFile(nextSibling);
        }
    } else {
        delete fileConfig;
    }
}

void File::processFile(const QDomNode &fileNode)
{
    File *file = new File(m_parentProjectDoc);
    file->processNode(fileNode);
    m_files.append(file);

    // process next sibling
    QDomNode nextSibling = fileNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("FileConfiguration"))
            processFileConfiguration(nextSibling);
        else
            processFile(nextSibling);
    }
}

void File::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("RelativePath"))
                m_relativePath = domElement.value();

            else
                m_attributeContainer->setAttribute(domElement.name(), domElement.value());
        }
    }
}

void File::copyAllNonDefaultToolAtributes(IConfiguration *fileConfig, IConfiguration *projConfig)
{
    if (fileConfig && projConfig &&
            fileConfig->tools() && fileConfig->tools()->configurationBuildTools() &&
            projConfig->tools() && projConfig->tools()->configurationBuildTools()
            ) {
        IConfigurationBuildTool *tool = fileConfig->tools()->configurationBuildTools()->tool(0);

        if (tool && tool->toolDescription()) {
            IConfigurationBuildTool *projToolCopy = projConfig->tools()->configurationBuildTools()->
                    tool(tool->toolDescription()->toolKey());

            if (projToolCopy) {
                ISectionContainer *secCont = tool->sectionContainer();
                ISectionContainer *projSecCont = projToolCopy->sectionContainer();

                if (secCont && projSecCont) {

                    for (int i = 0; i < secCont->sectionCount(); ++i) {
                        IToolSection *toolSec = secCont->section(i);

                        if (toolSec && toolSec->sectionDescription()) {
                            IToolSection *projSec = projSecCont->section(toolSec->sectionDescription()->displayName());
                            copyAllNonDefaultToolAtributes(toolSec, projSec);
                        }
                    }
                }
            }
        }
    }
}

void File::copyAllNonDefaultToolAtributes(IToolSection *fileSec, IToolSection *projSec)
{
    if (fileSec && projSec &&
            fileSec->sectionDescription() && projSec->sectionDescription()) {
        IToolAttributeContainer *attrCont = fileSec->attributeContainer();
        IToolAttributeContainer *projAttrCont = projSec->attributeContainer();

        if (attrCont && projAttrCont) {
            for (int i = 0; i < projAttrCont->toolAttributeCount(); ++i) {
                IToolAttribute *projToolAttr = projAttrCont->toolAttribute(i);

                if (projToolAttr && projToolAttr->descriptionDataItem()) {
                    IToolAttribute *toolAttr = attrCont->toolAttribute(projToolAttr->descriptionDataItem()->key());
                    if (toolAttr && !toolAttr->isUsed() && projToolAttr && projToolAttr->isUsed())
                        toolAttr->setValue(projToolAttr->value());
                }
            }
        }
    }
}

void File::leaveOnlyCppTool(IConfiguration *config)
{
    if (!config || !config->tools() || !config->tools()->configurationBuildTools())
        return;

    int i = 0;
    while (config->tools()->configurationBuildTools()->toolCount() > 1)
    {
        IConfigurationBuildTool *tool = config->tools()->configurationBuildTools()->tool(i);

        if (tool->toolDescription()->toolKey() != QLatin1String(ToolConstants::strVCCLCompilerTool)) {
            config->tools()->configurationBuildTools()->removeTool(tool);
            delete tool;
        }

        else
            ++i;
    }
}

void File::copyProjectConfigs()
{
    if (!m_parentProjectDoc || !m_parentProjectDoc->configurations() ||
            !m_parentProjectDoc->configurations()->configurationContainer())
        return;

    ConfigurationContainer *configContainer = m_parentProjectDoc->configurations()->configurationContainer();
    for (int i = 0; i < configContainer->configurationCount(); ++i) {
        IConfiguration *config = configContainer->configuration(i);

        if (config) {
            FileBuildConfiguration *newConfig = FileBuildConfiguration::createFromProjectConfig(static_cast<Configuration*>(config), m_parentProjectDoc);
            leaveOnlyCppTool(newConfig);
            m_configurationContainer->addConfiguration(newConfig);
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
