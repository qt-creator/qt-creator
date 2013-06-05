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
#include "vcprojectdocument.h"

#include <QDomElement>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QVBoxLayout>

#include "../widgets/projectsettingswidget.h"
#include "../widgets/configurationswidgets.h"

namespace VcProjectManager {
namespace Internal {

VcProjectDocument::~VcProjectDocument()
{
}

void VcProjectDocument::readFromXMLDomDocument(const QDomNode &domDoc)
{
    QDomNode node = domDoc.firstChild(); // xml

    parseProcessingInstruction(node.toProcessingInstruction());

    node = node.nextSibling(); // Visual studio
    processVisualStudioNode(node.toElement());
    node = node.firstChild();
    processNode(node);
}

bool VcProjectDocument::saveToFile(const QString &filePath) const
{
    QDomDocument domDoc;
    toXMLDomNode(domDoc);

    QFile outFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug( "VcProjectDocument::saveToFile::Failed to open file for writing." );
        return false;
    }

    QTextStream stream( &outFile );
    stream << domDoc.toString();
    outFile.close();
    return true;
}

VcDocConstants::DocumentVersion VcProjectDocument::documentVersion() const
{
    return m_documentVersion;
}

QString VcProjectDocument::filePath() const
{
    return m_filePath;
}

void VcProjectDocument::allProjectFiles(QStringList &sl) const
{
    m_files->allProjectFiles(sl);
}

QString VcProjectDocument::fileRelativePath(const QString &filePath)
{
    QString relativePath = QFileInfo(m_filePath).absoluteDir().relativeFilePath(filePath).replace(QLatin1String("/"), QLatin1String("\\"));

    if (!relativePath.startsWith(QLatin1String("..")))
        relativePath.prepend(QLatin1String(".\\"));

    return relativePath;
}

Configurations::Ptr VcProjectDocument::configurations() const
{
    return m_configurations;
}

Platforms::Ptr VcProjectDocument::platforms() const
{
    return m_platforms;
}

Files::Ptr VcProjectDocument::files() const
{
    return m_files;
}

Globals::Ptr VcProjectDocument::globals() const
{
    return m_globals;
}

References::Ptr VcProjectDocument::references() const
{
    return m_references;
}

VcProjectDocument::VcProjectDocument(const QString &filePath)
    : IVcProjectXMLNode(),
      m_filePath(filePath),
      m_projectType(QLatin1String("Visual C++")),
      m_platforms(Platforms::Ptr(new Platforms)),
      m_globals(Globals::Ptr(new Globals))
{
}

VcProjectDocument::VcProjectDocument(const VcProjectDocument &vcDoc)
{
    m_documentVersion = vcDoc.m_documentVersion;
    m_filePath = vcDoc.m_filePath;
    m_projectType = vcDoc.m_projectType;
    m_version = vcDoc.m_version;
    m_projectGUID = vcDoc.m_projectGUID;
    m_rootNamespace = vcDoc.m_rootNamespace;
    m_keyword = vcDoc.m_keyword;
    m_name = vcDoc.m_name;

    m_anyAttribute = vcDoc.m_anyAttribute;

    // <?xml part
    m_processingInstructionData = vcDoc.m_processingInstructionData;
    m_processingInstructionTarget = vcDoc.m_processingInstructionTarget;

    m_platforms = Platforms::Ptr(new Platforms(*(vcDoc.m_platforms)));
    m_configurations = Configurations::Ptr(new Configurations(*(vcDoc.m_configurations)));
    m_files = Files::Ptr(vcDoc.m_files->clone());
    m_references = References::Ptr(new References(*(vcDoc.m_references)));
    m_globals = Globals::Ptr(new Globals(*(vcDoc.m_globals)));
}

VcProjectDocument &VcProjectDocument::operator =(const VcProjectDocument &vcDoc)
{
    if (this != &vcDoc) {
        m_documentVersion = vcDoc.m_documentVersion;
        m_filePath = vcDoc.m_filePath;

        m_projectType = vcDoc.m_projectType;
        m_version = vcDoc.m_version;
        m_projectGUID = vcDoc.m_projectGUID;
        m_rootNamespace = vcDoc.m_rootNamespace;
        m_keyword = vcDoc.m_keyword;
        m_name = vcDoc.m_name;
        m_anyAttribute = vcDoc.m_anyAttribute;

        // <?xml part
        m_processingInstructionData = vcDoc.m_processingInstructionData;
        m_processingInstructionTarget = vcDoc.m_processingInstructionTarget;

        m_platforms = Platforms::Ptr(new Platforms(*(vcDoc.m_platforms)));
        m_configurations = Configurations::Ptr(new Configurations(*(vcDoc.m_configurations)));
        m_files = Files::Ptr(vcDoc.m_files->clone());
        m_references = References::Ptr(new References(*(vcDoc.m_references)));
        m_globals = Globals::Ptr(new Globals(*(vcDoc.m_globals)));
    }

    return *this;
}

void VcProjectDocument::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeName() == QLatin1String("Platforms"))
        m_platforms->processNode(node);

    else if (node.nodeName() == QLatin1String("Configurations"))
        m_configurations->processNode(node);

    else if (node.nodeName() == QLatin1String("References"))
        m_references->processNode(node);

    else if (node.nodeName() == QLatin1String("Files"))
        m_files->processNode(node);

    else if (node.nodeName() == QLatin1String("Globals"))
        m_globals->processNode(node);

    processNode(node.nextSibling());
}

void VcProjectDocument::processNodeAttributes(const QDomElement &element)
{
    Q_UNUSED(element)
}

void VcProjectDocument::parseProcessingInstruction(const QDomProcessingInstruction &processingInstruction)
{
    QStringList data = processingInstruction.data().split(QLatin1Char(' '));
    foreach (QString dataElement, data) {
        QStringList sl = dataElement.split(QLatin1Char('='));

        if (sl.size() == 2) {
            QString value = sl[1];
            value = value.replace(QLatin1String("\'"), QString());
            m_processingInstructionData.insert(sl[0].trimmed(), value.trimmed());
        }
    }

    m_processingInstructionTarget = processingInstruction.target();
}

void VcProjectDocument::processVisualStudioNode(const QDomElement &vsNode)
{
    QDomNamedNodeMap namedNodeMap = vsNode.attributes();

    for (int i = 0; i < namedNodeMap.count(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("ProjectType"))
                m_projectType = domElement.value();

            else if (domElement.name() == QLatin1String("Version"))
                m_version = domElement.value();

            else if (domElement.name() == QLatin1String("ProjectGUID"))
                m_projectGUID = domElement.value();

            else if (domElement.name() == QLatin1String("RootNamespace"))
                m_rootNamespace = domElement.value();

            else if (domElement.name() == QLatin1String("Keyword"))
                m_keyword = domElement.value();

            else if (domElement.name() == QLatin1String("Name"))
                m_name = domElement.value();

            else
                m_anyAttribute.insert(domElement.name(), domElement.value());
        }
    }
}

QDomNode VcProjectDocument::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QString processingData;
    QHashIterator<QString, QString> it(m_processingInstructionData);

    while (it.hasNext()) {
        it.next();
        processingData.append(it.key());
        processingData.append(QLatin1Char('='));
        processingData.append(QLatin1String("\"") + it.value() + QLatin1String("\""));
        processingData.append(QLatin1Char(' '));
    }

    QDomProcessingInstruction processingInstruction = domXMLDocument.createProcessingInstruction(QLatin1String("xml"), processingData);
    domXMLDocument.appendChild(processingInstruction);
    domXMLDocument.appendChild(toVcDocumentElement(domXMLDocument));

    return domXMLDocument;
}

QDomElement VcProjectDocument::toVcDocumentElement(QDomDocument &domXMLDocument) const
{
    QDomElement vcDocNode = domXMLDocument.createElement(QLatin1String("VisualStudioProject"));

    if (!m_name.isEmpty())
        vcDocNode.setAttribute(QLatin1String("Name"), m_name);

    if (!m_projectType.isEmpty())
        vcDocNode.setAttribute(QLatin1String("ProjectType"), m_projectType);

    if (!m_version.isEmpty())
        vcDocNode.setAttribute(QLatin1String("Version"), m_version);

    if (!m_projectGUID.isEmpty())
        vcDocNode.setAttribute(QLatin1String("ProjectGUID"), m_projectGUID);

    if (!m_rootNamespace.isEmpty())
        vcDocNode.setAttribute(QLatin1String("RootNamespace"), m_rootNamespace);

    if (!m_keyword.isEmpty())
        vcDocNode.setAttribute(QLatin1String("Keyword"), m_keyword);

    QHashIterator<QString, QString> itAttr(m_anyAttribute);

    while (itAttr.hasNext()) {
        itAttr.next();
        vcDocNode.setAttribute(itAttr.key(), itAttr.value());
    }

    if (!m_platforms->isEmpty())
        vcDocNode.appendChild(m_platforms->toXMLDomNode(domXMLDocument));

    if (!m_configurations->isEmpty())
        vcDocNode.appendChild(m_configurations->toXMLDomNode(domXMLDocument));

    if (!m_files->isEmpty())
        vcDocNode.appendChild(m_files->toXMLDomNode(domXMLDocument));

    if (!m_references->isEmpty())
        vcDocNode.appendChild(m_references->toXMLDomNode(domXMLDocument));

    if (!m_globals->isEmpty())
        vcDocNode.appendChild(m_globals->toXMLDomNode(domXMLDocument));

    return vcDocNode;
}


VcProjectDocument2003::VcProjectDocument2003(const VcProjectDocument2003 &vcDoc)
    : VcProjectDocument(vcDoc)
{
}

VcProjectDocument2003 &VcProjectDocument2003::operator =(const VcProjectDocument2003 &vcDoc)
{
    VcProjectDocument::operator =(vcDoc);
    return *this;
}

VcProjectDocument2003::~VcProjectDocument2003()
{
}

VcNodeWidget *VcProjectDocument2003::createSettingsWidget()
{
    return new VcProjectDocument2003Widget(this);
}

MsBuildInformation::MsBuildVersion VcProjectDocument2003::minSupportedMsBuildVersion() const
{
    return MsBuildInformation::MSBUILD_V_2_0;
}

MsBuildInformation::MsBuildVersion VcProjectDocument2003::maxSupportedMsBuildVersion() const
{
    return MsBuildInformation::MSBUILD_V_3_5;
}

VcProjectDocument2003::VcProjectDocument2003(const QString &filePath)
    : VcProjectDocument(filePath)
{
}

void VcProjectDocument2003::init()
{
    m_documentVersion = VcDocConstants::DV_MSVC_2003;
    m_files = Files::Ptr(new Files2003(this));
    m_configurations = Configurations::Ptr(new Configurations(this));
    m_references = References::Ptr(new References(m_documentVersion));
}


VcProjectDocumentWidget::VcProjectDocumentWidget(VcProjectDocument *vcDoc)
    : m_vcDoc(vcDoc)
{
    ProjectSettingsWidget *projectSettingsWidget = new ProjectSettingsWidget(m_vcDoc, this);

    // add Configurations
    m_configurationsWidget = static_cast<ConfigurationsBaseWidget *>(m_vcDoc->configurations()->createSettingsWidget());
    projectSettingsWidget->addWidget(tr("Configurations"), m_configurationsWidget);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(projectSettingsWidget);
    setLayout(layout);

    connect(projectSettingsWidget, SIGNAL(okButtonClicked()), this, SLOT(onOkButtonClicked()));
    connect(projectSettingsWidget, SIGNAL(cancelButtonClicked()), this, SLOT(onCancelButtonClicked()));
}

VcProjectDocumentWidget::~VcProjectDocumentWidget()
{
}

void VcProjectDocumentWidget::saveData()
{
    m_configurationsWidget->saveData();
}

void VcProjectDocumentWidget::onOkButtonClicked()
{
    saveData();
    hide();
    emit accepted();
    deleteLater();
}

void VcProjectDocumentWidget::onCancelButtonClicked()
{
    hide();
    deleteLater();
}


VcProjectDocument2003Widget::VcProjectDocument2003Widget(VcProjectDocument2003 *vcDoc)
    : VcProjectDocumentWidget(vcDoc)
{
}

VcProjectDocument2003Widget::~VcProjectDocument2003Widget()
{
}


VcProjectDocument2005::VcProjectDocument2005(const VcProjectDocument2005 &vcDoc)
    : VcProjectDocument2003(vcDoc)
{
    m_toolFiles = ToolFiles::Ptr(new ToolFiles(*(vcDoc.m_toolFiles)));
}

VcProjectDocument2005 &VcProjectDocument2005::operator =(const VcProjectDocument2005 &vcDoc)
{
    if (this != &vcDoc) {
        VcProjectDocument2003::operator =(vcDoc);
        m_toolFiles = ToolFiles::Ptr(new ToolFiles(*(vcDoc.m_toolFiles)));
    }

    return *this;
}

VcProjectDocument2005::~VcProjectDocument2005()
{
}

VcNodeWidget *VcProjectDocument2005::createSettingsWidget()
{
    return new VcProjectDocument2005Widget(this);
}

MsBuildInformation::MsBuildVersion VcProjectDocument2005::minSupportedMsBuildVersion() const
{
    return MsBuildInformation::MSBUILD_V_3_0;
}

MsBuildInformation::MsBuildVersion VcProjectDocument2005::maxSupportedMsBuildVersion() const
{
    return MsBuildInformation::MSBUILD_V_3_5;
}

ToolFiles::Ptr VcProjectDocument2005::toolFiles() const
{
    return m_toolFiles;
}

VcProjectDocument2005::VcProjectDocument2005(const QString &filePath)
    : VcProjectDocument2003(filePath),
      m_toolFiles(ToolFiles::Ptr(new ToolFiles))
{
}

void VcProjectDocument2005::init()
{
    m_documentVersion = VcDocConstants::DV_MSVC_2005;
    m_files = Files::Ptr(new Files2005(this));
    m_configurations = Configurations::Ptr(new Configurations(this));
    m_references = References::Ptr(new References(m_documentVersion));
}

void VcProjectDocument2005::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeName() == QLatin1String("Platforms"))
        m_platforms->processNode(node);

    else if (node.nodeName() == QLatin1String("Configurations"))
        m_configurations->processNode(node);

    else if (node.nodeName() == QLatin1String("References"))
        m_references->processNode(node);

    else if (node.nodeName() == QLatin1String("Files"))
        m_files->processNode(node);

    else if (node.nodeName() == QLatin1String("Globals"))
        m_globals->processNode(node);

    else if (node.nodeName() == QLatin1String("ToolFiles"))
        m_toolFiles->processNode(node);

    processNode(node.nextSibling());
}

QDomElement VcProjectDocument2005::toVcDocumentElement(QDomDocument &domXMLDocument) const
{
    QDomElement vcDocNode = VcProjectDocument2003::toVcDocumentElement(domXMLDocument);

    if (!m_toolFiles->isEmpty())
        vcDocNode.appendChild(m_toolFiles->toXMLDomNode(domXMLDocument));

    return vcDocNode;
}


VcProjectDocument2005Widget::VcProjectDocument2005Widget(VcProjectDocument2005 *vcDoc)
    : VcProjectDocumentWidget(vcDoc)
{
}

VcProjectDocument2005Widget::~VcProjectDocument2005Widget()
{
}


VcProjectDocument2008::VcProjectDocument2008(const VcProjectDocument2008 &vcDoc)
    : VcProjectDocument2005(vcDoc)
{
    m_publishingData = PublishingData::Ptr(new PublishingData(*(vcDoc.m_publishingData)));

    m_assemblyReferenceSearchPaths = vcDoc.m_assemblyReferenceSearchPaths;
    m_manifestKeyFile = vcDoc.m_manifestKeyFile;
    m_manifestCertificateThumbprint = vcDoc.m_manifestCertificateThumbprint;
    m_manifestTimestampURL = vcDoc.m_manifestTimestampURL;
    m_signManifests = vcDoc.m_signManifests;
    m_signAssembly = vcDoc.m_signAssembly;
    m_assemblyOriginatorKeyFile = vcDoc.m_assemblyOriginatorKeyFile;
    m_delaySign = vcDoc.m_delaySign;
    m_generateManifests = vcDoc.m_generateManifests;
    m_targetZone = vcDoc.m_targetZone;
    m_excludedPermissions = vcDoc.m_excludedPermissions;
    m_targetFrameworkVersion = vcDoc.m_targetFrameworkVersion;
}

VcProjectDocument2008 &VcProjectDocument2008::operator=(const VcProjectDocument2008 &vcDoc)
{
    if (this != &vcDoc) {
        VcProjectDocument2005::operator =(vcDoc);

        m_publishingData = PublishingData::Ptr(new PublishingData(*(vcDoc.m_publishingData)));

        m_assemblyReferenceSearchPaths = vcDoc.m_assemblyReferenceSearchPaths;
        m_manifestKeyFile = vcDoc.m_manifestKeyFile;
        m_manifestCertificateThumbprint = vcDoc.m_manifestCertificateThumbprint;
        m_manifestTimestampURL = vcDoc.m_manifestTimestampURL;
        m_signManifests = vcDoc.m_signManifests;
        m_signAssembly = vcDoc.m_signAssembly;
        m_assemblyOriginatorKeyFile = vcDoc.m_assemblyOriginatorKeyFile;
        m_delaySign = vcDoc.m_delaySign;
        m_generateManifests = vcDoc.m_generateManifests;
        m_targetZone = vcDoc.m_targetZone;
        m_excludedPermissions = vcDoc.m_excludedPermissions;
        m_targetFrameworkVersion = vcDoc.m_targetFrameworkVersion;
    }
    return *this;
}

VcProjectDocument2008::~VcProjectDocument2008()
{
}

VcNodeWidget *VcProjectDocument2008::createSettingsWidget()
{
    return new VcProjectDocument2008Widget(this);
}

MsBuildInformation::MsBuildVersion VcProjectDocument2008::minSupportedMsBuildVersion() const
{
    return MsBuildInformation::MSBUILD_V_3_5;
}

MsBuildInformation::MsBuildVersion VcProjectDocument2008::maxSupportedMsBuildVersion() const
{
    return MsBuildInformation::MSBUILD_V_3_5;
}

PublishingData::Ptr VcProjectDocument2008::publishingData() const
{
    return m_publishingData;
}

VcProjectDocument2008::VcProjectDocument2008(const QString &filePath)
    : VcProjectDocument2005(filePath),
      m_publishingData(PublishingData::Ptr(new PublishingData))
{
}

void VcProjectDocument2008::init()
{
    m_documentVersion = VcDocConstants::DV_MSVC_2008;
    m_files = Files::Ptr(new Files2008(this));
    m_configurations = Configurations::Ptr(new Configurations(this));
    m_references = References::Ptr(new References(m_documentVersion));
}

void VcProjectDocument2008::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeName() == QLatin1String("Platforms"))
        m_platforms->processNode(node);

    else if (node.nodeName() == QLatin1String("Configurations"))
        m_configurations->processNode(node);

    else if (node.nodeName() == QLatin1String("References"))
        m_references->processNode(node);

    else if (node.nodeName() == QLatin1String("Files"))
        m_files->processNode(node);

    else if (node.nodeName() == QLatin1String("Globals"))
        m_globals->processNode(node);

    else if (node.nodeName() == QLatin1String("ToolFiles"))
        m_toolFiles->processNode(node);

    else if (node.nodeName() == QLatin1String("PublishingData"))
        m_publishingData->processNode(node);

    processNode(node.nextSibling());
}

void VcProjectDocument2008::processVisualStudioNode(const QDomElement &vsNode)
{
    VcProjectDocument2005::processVisualStudioNode(vsNode);

    QDomNamedNodeMap namedNodeMap = vsNode.attributes();

    for (int i = 0; i < namedNodeMap.count(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("AssemblyReferenceSearchPaths"))
                m_assemblyReferenceSearchPaths = domElement.value();

            else if (domElement.name() == QLatin1String("ManifestKeyFile"))
                m_manifestKeyFile = domElement.value();

            else if (domElement.name() == QLatin1String("ManifestCertificateThumbprint"))
                m_manifestCertificateThumbprint = domElement.value();

            else if (domElement.name() == QLatin1String("ManifestTimestampURL"))
                m_manifestTimestampURL = domElement.value();

            else if (domElement.name() == QLatin1String("SignManifests")) {
                if (domElement.value() == QLatin1String("false"))
                    m_signManifests = false;
                else
                    m_signManifests = true;
            }

            else if (domElement.name() == QLatin1String("SignAssembly")) {
                if (domElement.value() == QLatin1String("false"))
                    m_signAssembly = false;
                else
                    m_signAssembly = true;
            }

            else if (domElement.name() == QLatin1String("AssemblyOriginatorKeyFile"))
                m_assemblyOriginatorKeyFile = domElement.value();

            else if (domElement.name() == QLatin1String("DelaySign")) {
                if (domElement.value() == QLatin1String("false"))
                    m_delaySign = false;
                else
                    m_delaySign = true;
            }

            else if (domElement.name() == QLatin1String("DelaySign")) {
                if (domElement.value() == QLatin1String("GenerateManifests"))
                    m_generateManifests = false;
                else
                    m_generateManifests = true;
            }

            else if (domElement.name() == QLatin1String("TargetZone"))
                m_targetZone = domElement.value();

            else if (domElement.name() == QLatin1String("ExcludedPermissions"))
                m_excludedPermissions = domElement.value();

            else if (domElement.name() == QLatin1String("TargetFrameworkVersion"))
                m_targetFrameworkVersion = domElement.value();
        }
    }
}

QDomElement VcProjectDocument2008::toVcDocumentElement(QDomDocument &domXMLDocument) const
{
    QDomElement vcDocNode = VcProjectDocument2005::toVcDocumentElement(domXMLDocument);

    if (!m_assemblyReferenceSearchPaths.isEmpty())
        vcDocNode.setAttribute(QLatin1String("AssemblyReferenceSearchPaths"), m_assemblyReferenceSearchPaths);

    if (!m_manifestKeyFile.isEmpty())
        vcDocNode.setAttribute(QLatin1String("ManifestKeyFile"), m_manifestKeyFile);

    if (!m_manifestCertificateThumbprint.isEmpty())
        vcDocNode.setAttribute(QLatin1String("ManifestCertificateThumbprint"), m_manifestCertificateThumbprint);

    if (!m_manifestTimestampURL.isEmpty())
        vcDocNode.setAttribute(QLatin1String("ManifestTimestampURL"), m_manifestTimestampURL);

    vcDocNode.setAttribute(QLatin1String("SignManifests"), QVariant(m_signManifests).toString());
    vcDocNode.setAttribute(QLatin1String("SignAssembly"), QVariant(m_signAssembly).toString());

    if (!m_assemblyOriginatorKeyFile.isEmpty())
        vcDocNode.setAttribute(QLatin1String("AssemblyOriginatorKeyFile"), m_assemblyOriginatorKeyFile);

    vcDocNode.setAttribute(QLatin1String("DelaySign"), QVariant(m_delaySign).toString());
    vcDocNode.setAttribute(QLatin1String("GenerateManifests"), QVariant(m_generateManifests).toString());

    if (!m_targetZone.isEmpty())
        vcDocNode.setAttribute(QLatin1String("TargetZone"), m_targetZone);

    if (!m_excludedPermissions.isEmpty())
        vcDocNode.setAttribute(QLatin1String("ExcludedPermissions"), m_excludedPermissions);

    if (!m_targetFrameworkVersion.isEmpty())
        vcDocNode.setAttribute(QLatin1String("TargetFrameworkVersion"), m_targetFrameworkVersion);

    if (!m_publishingData->isEmpty())
        vcDocNode.appendChild(m_publishingData->toXMLDomNode(domXMLDocument));

    return vcDocNode;
}


VcProjectDocument2008Widget::VcProjectDocument2008Widget(VcProjectDocument2008 *vcDoc)
    : VcProjectDocumentWidget(vcDoc)
{
}

VcProjectDocument2008Widget::~VcProjectDocument2008Widget()
{
}


VcProjectDocument *VcProjectDocumentFactory::create(const QString &filePath, VcDocConstants::DocumentVersion version)
{
    VcProjectDocument *vcDoc = 0;

    switch (version) {
    case VcDocConstants::DV_MSVC_2003:
        vcDoc = new VcProjectDocument2003(filePath);
        break;
    case VcDocConstants::DV_MSVC_2005:
        vcDoc = new VcProjectDocument2005(filePath);
        break;
    case VcDocConstants::DV_MSVC_2008:
        vcDoc = new VcProjectDocument2008(filePath);
        break;
    }

    if (vcDoc)
        vcDoc->init();
    return vcDoc;
}

} // namespace Internal
} // namespace VcProjectManager
