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

#ifndef VCPROJECTMANAGER_INTERNAL_VCPROJECTDOCUMENT_H
#define VCPROJECTMANAGER_INTERNAL_VCPROJECTDOCUMENT_H

#include "../interfaces/ivisualstudioproject.h"

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

class VcProjectDocument : public IVisualStudioProject
{
    friend class VcProjectDocumentFactory;

public:
    VcProjectDocument(const QString &filePath, VcDocConstants::DocumentVersion docVersion);
    VcProjectDocument(const VcProjectDocument &vcDoc);
    VcProjectDocument& operator=(const VcProjectDocument &vcDoc);
    virtual ~VcProjectDocument();
    void processNode(const QDomNode &domDoc);

    bool saveToFile(const QString &filePath) const;
    VcDocConstants::DocumentVersion documentVersion() const;
    QString filePath() const;

    IConfigurations *configurations() const;
    IFiles *files() const;
    IGlobals *globals() const;
    IPlatforms *platforms() const;
    IReferences *referencess() const;
    IToolFiles *toolFiles() const;
    IPublishingData *publishingData() const;
    IAttributeContainer* attributeContainer() const;
    VcNodeWidget *createSettingsWidget();

protected:
    void processDocumentNode(const QDomNode &node);
    void processDocumentAttributes(const QDomElement &vsNode);
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;
    QDomElement toVcDocumentElement(QDomDocument &domXMLDocument) const;

    void parseProcessingInstruction(const QDomProcessingInstruction &processingInstruction);

    QString m_filePath; // used to store path to a file

    // <?xml part
    QHash<QString, QString> m_processingInstructionData;
    QString m_processingInstructionTarget;

    VcDocConstants::DocumentVersion m_documentVersion;
    Platforms *m_platforms;
    Configurations *m_configurations;
    Files *m_files;
    References *m_references;
    Globals *m_globals;
    ToolFiles *m_toolFiles;
    PublishingData *m_publishingData;
    GeneralAttributeContainer* m_attributeContainer;
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

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCPROJECTDOCUMENT_H
