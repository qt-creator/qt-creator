/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#include "file.h"

#include "vcprojectdocument.h"
#include "configurationcontainer.h"
#include "generalattributecontainer.h"
#include "../widgets/configurationseditwidget.h"
#include "filebuildconfiguration.h"
#include "tools/toolattributes/tooldescriptiondatamanager.h"
#include "tools/tool_constants.h"
#include "../interfaces/itooldescription.h"
#include "../interfaces/itools.h"
#include "../interfaces/iconfigurationbuildtool.h"
#include "../interfaces/iconfigurationbuildtools.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>

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

    foreach (const File::Ptr &f, file.m_files)
        m_files.append(File::Ptr(new File(*f)));
}

File &File::operator =(const File &file)
{
    if (this != &file) {
        m_parentProjectDoc = file.m_parentProjectDoc;
        m_relativePath = file.m_relativePath;
        *m_configurationContainer = *(file.m_configurationContainer);
        *m_attributeContainer = *(file.m_attributeContainer);

        m_files.clear();

        foreach (const File::Ptr &f, file.m_files)
            m_files.append(File::Ptr(new File(*f)));
    }
    return *this;
}

File::~File()
{
    m_files.clear();
    delete m_configurationContainer;
    delete m_attributeContainer;
}

void File::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeType() == QDomNode::ElementNode)
        processNodeAttributes(node.toElement());

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
    return new ConfigurationsEditWidget(m_parentProjectDoc, m_configurationContainer);
}

QDomNode File::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement fileNode = domXMLDocument.createElement(QLatin1String("File"));

    fileNode.setAttribute(QLatin1String("RelativePath"), m_relativePath);

    foreach (const File::Ptr &file, m_files)
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
    const Core::MimeDatabase *mdb = Core::ICore::mimeDatabase();
    QString mimeType = mdb->findByFile(canonicalPath()).type();

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

IConfiguration *File::createDefaultBuildConfiguration(const QString &fullConfigName) const
{
    IConfiguration *config = new FileBuildConfiguration;
    config->setFullName(fullConfigName);

    ToolDescriptionDataManager *tDDM = ToolDescriptionDataManager::instance();
    IToolDescription *toolDesc = tDDM->toolDescription(QLatin1String(ToolConstants::strVCCLCompilerTool));

    if (toolDesc) {
        IConfigurationBuildTool *tool = toolDesc->createTool();
        config->tools()->configurationBuildTools()->addTool(tool);
    }

    return config;
}

void File::processFileConfiguration(const QDomNode &fileConfigNode)
{
    IConfiguration *fileConfig = new FileBuildConfiguration();
    fileConfig->processNode(fileConfigNode);
    m_configurationContainer->addConfiguration(fileConfig);

    // process next sibling
    QDomNode nextSibling = fileConfigNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("FileConfiguration"))
            processFileConfiguration(nextSibling);
        else
            processFile(nextSibling);
    }
}

void File::processFile(const QDomNode &fileNode)
{
    File::Ptr file(new File(m_parentProjectDoc));
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

} // namespace Internal
} // namespace VcProjectManager
