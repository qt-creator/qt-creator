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

#include "configurationsfactory.h"
#include "vcprojectdocument.h"
#include "configurationcontainer.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>

namespace VcProjectManager {
namespace Internal {

File::File(VcProjectDocument *parentProjectDoc)
    : m_parentProjectDoc(parentProjectDoc)
{
    m_configurationContainer = new ConfigurationContainer;
}

File::File(const File &file)
{
    m_parentProjectDoc = file.m_parentProjectDoc;
    m_relativePath = file.m_relativePath;
    m_configurationContainer = new ConfigurationContainer;
    *m_configurationContainer = *(file.m_configurationContainer);

    foreach (const File::Ptr &f, file.m_files)
        m_files.append(File::Ptr(new File(*f)));
}

File &File::operator =(const File &file)
{
    if (this != &file) {
        m_parentProjectDoc = file.m_parentProjectDoc;
        m_relativePath = file.m_relativePath;
        *m_configurationContainer = *(file.m_configurationContainer);

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
    return 0;
}

QDomNode File::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement fileNode = domXMLDocument.createElement(QLatin1String("File"));

    fileNode.setAttribute(QLatin1String("RelativePath"), m_relativePath);

    QHashIterator<QString, QString> it(m_anyAttribute);

    while (it.hasNext()) {
        it.next();
        fileNode.setAttribute(it.key(), it.value());
    }

    foreach (const File::Ptr &file, m_files)
        fileNode.appendChild(file->toXMLDomNode(domXMLDocument));

    m_configurationContainer->appendToXMLNode(fileNode, domXMLDocument);

    return fileNode;
}

ConfigurationContainer *File::configurationContainer() const
{
    return m_configurationContainer;
}

void File::addFile(File::Ptr file)
{
    if (m_files.contains(file))
        return;
    m_files.append(file);
}

void File::removeFile(File::Ptr file)
{
    if (m_files.contains(file))
        m_files.removeAll(file);
}

QString File::attributeValue(const QString &attributeName) const
{
    return m_anyAttribute.value(attributeName);
}

void File::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_anyAttribute.insert(attributeName, attributeValue);
}

void File::clearAttribute(const QString &attributeName)
{
    // no need to clear the attribute's value if attribute isn't present
    if (m_anyAttribute.contains(attributeName))
        m_anyAttribute.insert(attributeName, QString());
}

void File::removeAttribute(const QString &attributeName)
{
    m_anyAttribute.remove(attributeName);
}

QString File::relativePath() const
{
    return m_relativePath;
}

void File::setRelativePath(const QString &relativePath)
{
    m_relativePath = relativePath;
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

void File::processFileConfiguration(const QDomNode &fileConfigNode)
{
    IConfiguration *fileConfig = 0;
    if (m_parentProjectDoc->documentVersion() == VcDocConstants::DV_MSVC_2003)
        fileConfig = new Configuration2003(QLatin1String("FileConfiguration"));
    else if (m_parentProjectDoc->documentVersion() == VcDocConstants::DV_MSVC_2005)
        fileConfig = new Configuration2005(QLatin1String("FileConfiguration"));
    else if (m_parentProjectDoc->documentVersion() == VcDocConstants::DV_MSVC_2008)
        fileConfig = new Configuration2008(QLatin1String("FileConfiguration"));

    if (fileConfig) {
        fileConfig->processNode(fileConfigNode);
        m_configurationContainer->addConfiguration(fileConfig);
    }

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
                m_anyAttribute.insert(domElement.name(), domElement.value());
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
