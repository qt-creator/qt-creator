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

#ifndef VCPROJECTMANAGER_INTERNAL_VCPROJECTDOCUMENT_H
#define VCPROJECTMANAGER_INTERNAL_VCPROJECTDOCUMENT_H

#include "ivcprojectnodemodel.h"

#include "configurations.h"
#include "files.h"
#include "globals.h"
#include "../msbuildversionmanager.h"
#include "platforms.h"
#include "publishingdata.h"
#include "references.h"
#include "toolfiles.h"
#include "../widgets/vcnodewidget.h"

namespace VcProjectManager {
namespace Internal {

class ConfigurationsBaseWidget;

class VcProjectDocument : public IVcProjectXMLNode
{
    friend class VcProjectDocumentFactory;

public:
    virtual ~VcProjectDocument();

    void readFromXMLDomDocument(const QDomNode &domDoc);
    bool saveToFile(const QString &filePath) const;
    VcDocConstants::DocumentVersion documentVersion() const;
    QString filePath() const;
    void allProjectFiles(QStringList &sl) const;

    /*!
     * Implementation should return a minimal version of MS Build tool that is required to build a project.
     * For example VS 2008 project can be compiled with MS Build 3.0 and 3.5.
     * So the minimal MS Build version for VS 2008 projects is 3.0
     * \return A minimal supported Ms Build version that is required to compile a project.
     */
    virtual MsBuildInformation::MsBuildVersion minSupportedMsBuildVersion() const = 0;

    /*!
     * Implementation should return a max version of MS Build tool that can be used to build a project.
     * For example VS 2008 project can be compiled with MS Build 3.0 and 3.5.
     * So the maximum MS Build version for VS 2008 projects is 3.5
     * \return
     */
    virtual MsBuildInformation::MsBuildVersion maxSupportedMsBuildVersion() const = 0;

    // helper function, used to create a relative path to a file, path is relative to a project's file path
    QString fileRelativePath(const QString &filePath);

    Configurations::Ptr configurations() const;
    Platforms::Ptr platforms() const;
    Files::Ptr files() const;
    Globals::Ptr globals() const;
    References::Ptr references() const;

protected:
    VcProjectDocument(const QString &filePath);
    VcProjectDocument(const VcProjectDocument &vcDoc);
    VcProjectDocument& operator=(const VcProjectDocument &vcDoc);
    void processNode(const QDomNode &node);
    void processNodeAttributes(const QDomElement &element);
    virtual void processVisualStudioNode(const QDomElement &vsNode);
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;
    virtual QDomElement toVcDocumentElement(QDomDocument &domXMLDocument) const;

    /*!
     * Called to initialize VcProjectDocument after instance is created.
     * This is the place in which objects that represent Configurations, References, Globals etc. should be created.
     */
    virtual void init() = 0;

    void parseProcessingInstruction(const QDomProcessingInstruction &processingInstruction);

    QString m_filePath;
    QString m_projectType; // optional
    QString m_version;  // optional

    QString m_projectGUID; // optional
    QString m_rootNamespace; // optional
    QString m_keyword; // optional
    QString m_name; // optional

    QHash<QString, QString> m_anyAttribute;

    // <?xml part
    QHash<QString, QString> m_processingInstructionData;
    QString m_processingInstructionTarget;

    VcDocConstants::DocumentVersion m_documentVersion;
    Platforms::Ptr m_platforms;
    Configurations::Ptr m_configurations;
    Files::Ptr m_files;
    References::Ptr m_references;
    Globals::Ptr m_globals;
};

class VcProjectDocument2003 : public VcProjectDocument
{
    friend class VcProjectDocumentFactory;

public:
    VcProjectDocument2003(const VcProjectDocument2003 &vcDoc);
    VcProjectDocument2003& operator=(const VcProjectDocument2003 &vcDoc);
    ~VcProjectDocument2003();

    VcNodeWidget* createSettingsWidget();
    MsBuildInformation::MsBuildVersion minSupportedMsBuildVersion() const;
    MsBuildInformation::MsBuildVersion maxSupportedMsBuildVersion() const;

protected:
    VcProjectDocument2003(const QString &filePath);
    void init();
};


class VcProjectDocumentWidget : public VcNodeWidget
{
    Q_OBJECT

public:
    explicit VcProjectDocumentWidget(VcProjectDocument *vcDoc);
    ~VcProjectDocumentWidget();
    void saveData();

private slots:
    virtual void onOkButtonClicked();
    virtual void onCancelButtonClicked();

signals:
    void accepted();

protected:
    VcProjectDocument *m_vcDoc;
    ConfigurationsBaseWidget *m_configurationsWidget;
};


class VcProjectDocument2003Widget : public VcProjectDocumentWidget
{
    Q_OBJECT

public:
    explicit VcProjectDocument2003Widget(VcProjectDocument2003 *vcDoc);
    ~VcProjectDocument2003Widget();
};

class VcProjectDocument2005 : public VcProjectDocument2003
{
    friend class VcProjectDocumentFactory;

public:
    VcProjectDocument2005(const VcProjectDocument2005 &vcDoc);
    VcProjectDocument2005& operator=(const VcProjectDocument2005 &vcDoc);
    ~VcProjectDocument2005();

    VcNodeWidget* createSettingsWidget();
    MsBuildInformation::MsBuildVersion minSupportedMsBuildVersion() const;
    MsBuildInformation::MsBuildVersion maxSupportedMsBuildVersion() const;

    ToolFiles::Ptr toolFiles() const;

protected:
    VcProjectDocument2005(const QString &filePath);
    void init();
    void processNode(const QDomNode &node);
    QDomElement toVcDocumentElement(QDomDocument &domXMLDocument) const;

    ToolFiles::Ptr m_toolFiles;
};

class VcProjectDocument2005Widget : public VcProjectDocumentWidget
{
    Q_OBJECT

public:
    explicit VcProjectDocument2005Widget(VcProjectDocument2005 *vcDoc);
    ~VcProjectDocument2005Widget();
};

class VcProjectDocument2008 : public VcProjectDocument2005
{
    friend class VcProjectDocumentFactory;

public:
    VcProjectDocument2008(const VcProjectDocument2008 &vcDoc);
    VcProjectDocument2008& operator=(const VcProjectDocument2008 &vcDoc);
    ~VcProjectDocument2008();

    VcNodeWidget* createSettingsWidget();
    MsBuildInformation::MsBuildVersion minSupportedMsBuildVersion() const;
    MsBuildInformation::MsBuildVersion maxSupportedMsBuildVersion() const;

    PublishingData::Ptr publishingData() const;

protected:
    VcProjectDocument2008(const QString &filePath);
    void init();
    void processNode(const QDomNode &node);
    void processVisualStudioNode(const QDomElement &vsNode);
    QDomElement toVcDocumentElement(QDomDocument &domXMLDocument) const;

private:
    PublishingData::Ptr m_publishingData;

    QString m_assemblyReferenceSearchPaths;
    QString m_manifestKeyFile;
    QString m_manifestCertificateThumbprint;
    QString m_manifestTimestampURL;
    bool m_signManifests;
    bool m_signAssembly;
    QString m_assemblyOriginatorKeyFile;
    bool m_delaySign;
    bool m_generateManifests;
    QString m_targetZone;
    QString m_excludedPermissions;
    QString m_targetFrameworkVersion;
};

class VcProjectDocument2008Widget : public VcProjectDocumentWidget
{
    Q_OBJECT

public:
    explicit VcProjectDocument2008Widget(VcProjectDocument2008 *vcDoc);
    ~VcProjectDocument2008Widget();
};

class VcProjectDocumentFactory
{
public:
    static VcProjectDocument* create(const QString &filePath, VcDocConstants::DocumentVersion version);
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCPROJECTDOCUMENT_H
