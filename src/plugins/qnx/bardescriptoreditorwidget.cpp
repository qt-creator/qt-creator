/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "bardescriptoreditorwidget.h"

#include "qnxconstants.h"
#include "bardescriptoreditor.h"
#include "bardescriptoreditorassetswidget.h"
#include "bardescriptoreditorauthorinformationwidget.h"
#include "bardescriptoreditorentrypointwidget.h"
#include "bardescriptoreditorenvironmentwidget.h"
#include "bardescriptoreditorgeneralwidget.h"
#include "bardescriptoreditorpackageinformationwidget.h"
#include "bardescriptoreditorpermissionswidget.h"

#include <coreplugin/icore.h>
#include <projectexplorer/panelswidget.h>
#include <projectexplorer/propertiespanel.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>
#include <texteditor/plaintexteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/normalindenter.h>
#include <utils/qtcassert.h>

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorEditorWidget::BarDescriptorEditorWidget(BarDescriptorEditor *editor, QWidget *parent)
    : QStackedWidget(parent)
    , m_editor(editor)
{
    Core::IContext *myContext = new Core::IContext(this);
    myContext->setWidget(this);
    myContext->setContext(Core::Context(Constants::QNX_BAR_DESCRIPTOR_EDITOR_CONTEXT, TextEditor::Constants::C_TEXTEDITOR));
    Core::ICore::addContextObject(myContext);

    initGeneralPage();
    initApplicationPage();
    initAssetsPage();
    initSourcePage();

    setCurrentIndex(0);

    connect(m_entryPointWidget,         SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), barDescriptorDocument(), SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));
    connect(m_packageInformationWidget, SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), barDescriptorDocument(), SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));
    connect(m_authorInformationWidget,  SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), barDescriptorDocument(), SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));
    connect(m_generalWidget,            SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), barDescriptorDocument(), SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));
    connect(m_permissionsWidget,        SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), barDescriptorDocument(), SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));
    connect(m_environmentWidget,        SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), barDescriptorDocument(), SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));
    connect(m_assetsWidget,             SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), barDescriptorDocument(), SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));

    connect(barDescriptorDocument(), SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), m_entryPointWidget,         SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));
    connect(barDescriptorDocument(), SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), m_packageInformationWidget, SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));
    connect(barDescriptorDocument(), SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), m_authorInformationWidget,  SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));
    connect(barDescriptorDocument(), SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), m_generalWidget,            SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));
    connect(barDescriptorDocument(), SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), m_permissionsWidget,        SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));
    connect(barDescriptorDocument(), SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), m_environmentWidget,        SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));
    connect(barDescriptorDocument(), SIGNAL(changed(BarDescriptorDocument::Tag,QVariant)), m_assetsWidget,             SLOT(setValue(BarDescriptorDocument::Tag,QVariant)));

    connect(m_xmlSourceWidget, SIGNAL(textChanged()), this, SLOT(updateDocumentContent()));
    connect(barDescriptorDocument(), SIGNAL(changed()), this, SLOT(updateSourceView()));
}

void BarDescriptorEditorWidget::initGeneralPage()
{
    ProjectExplorer::PanelsWidget *generalPanel = new ProjectExplorer::PanelsWidget(this);
    initPanelSize(generalPanel);
    addWidget(generalPanel);

    // Entry-Point Text and Images
    ProjectExplorer::PropertiesPanel *entryPointPanel = new ProjectExplorer::PropertiesPanel;
    m_entryPointWidget = new BarDescriptorEditorEntryPointWidget;
    entryPointPanel->setDisplayName(tr("Entry-Point Text and Images"));
    entryPointPanel->setWidget(m_entryPointWidget);
    generalPanel->addPropertiesPanel(entryPointPanel);

    // Package Information
    ProjectExplorer::PropertiesPanel *packageInformationPanel = new ProjectExplorer::PropertiesPanel;
    m_packageInformationWidget = new BarDescriptorEditorPackageInformationWidget;
    packageInformationPanel->setDisplayName(tr("Package Information"));
    packageInformationPanel->setWidget(m_packageInformationWidget);
    generalPanel->addPropertiesPanel(packageInformationPanel);

    // Author information
    ProjectExplorer::PropertiesPanel *authorInformationPanel = new ProjectExplorer::PropertiesPanel;
    m_authorInformationWidget = new BarDescriptorEditorAuthorInformationWidget;
    authorInformationPanel->setDisplayName(tr("Author Information"));
    authorInformationPanel->setWidget(m_authorInformationWidget);
    generalPanel->addPropertiesPanel(authorInformationPanel);
}

void BarDescriptorEditorWidget::initApplicationPage()
{
    ProjectExplorer::PanelsWidget *applicationPanel = new ProjectExplorer::PanelsWidget(this);
    initPanelSize(applicationPanel);
    addWidget(applicationPanel);

    // General
    ProjectExplorer::PropertiesPanel *generalPanel = new ProjectExplorer::PropertiesPanel;
    m_generalWidget = new BarDescriptorEditorGeneralWidget;
    generalPanel->setDisplayName(tr("General"));
    generalPanel->setWidget(m_generalWidget);
    applicationPanel->addPropertiesPanel(generalPanel);

    //Permissions
    ProjectExplorer::PropertiesPanel *permissionsPanel = new ProjectExplorer::PropertiesPanel;
    m_permissionsWidget = new BarDescriptorEditorPermissionsWidget;
    permissionsPanel->setDisplayName(tr("Permissions"));
    permissionsPanel->setWidget(m_permissionsWidget);
    applicationPanel->addPropertiesPanel(permissionsPanel);

    // Environment
    ProjectExplorer::PropertiesPanel *environmentPanel = new ProjectExplorer::PropertiesPanel;
    m_environmentWidget = new BarDescriptorEditorEnvironmentWidget;
    environmentPanel->setDisplayName(tr("Environment"));
    environmentPanel->setWidget(m_environmentWidget);
    applicationPanel->addPropertiesPanel(environmentPanel);
}

void BarDescriptorEditorWidget::initAssetsPage()
{
    ProjectExplorer::PanelsWidget *assetsPanel = new ProjectExplorer::PanelsWidget(this);
    initPanelSize(assetsPanel);
    addWidget(assetsPanel);

    ProjectExplorer::PropertiesPanel *assetsPropertiesPanel = new ProjectExplorer::PropertiesPanel;
    m_assetsWidget = new BarDescriptorEditorAssetsWidget;
    assetsPropertiesPanel->setDisplayName(tr("Assets"));
    assetsPropertiesPanel->setWidget(m_assetsWidget);
    assetsPanel->addPropertiesPanel(assetsPropertiesPanel);

    m_entryPointWidget->setAssetsModel(m_assetsWidget->assetsModel());
    connect(m_entryPointWidget, SIGNAL(imageAdded(QString)), m_assetsWidget, SLOT(addAsset(QString)));
    connect(m_entryPointWidget, SIGNAL(imageRemoved(QString)), m_assetsWidget, SLOT(removeAsset(QString)));
}

void BarDescriptorEditorWidget::initSourcePage()
{
    auto doc = new TextEditor::PlainTextDocument;
    doc->setIndenter(new TextEditor::NormalIndenter);
    m_xmlSourceWidget = new TextEditor::PlainTextEditorWidget(doc, this);
    addWidget(m_xmlSourceWidget);

    TextEditor::TextEditorSettings::initializeEditor(m_xmlSourceWidget);
    m_xmlSourceWidget->configureMimeType(QLatin1String(Constants::QNX_BAR_DESCRIPTOR_MIME_TYPE));
}

void BarDescriptorEditorWidget::initPanelSize(ProjectExplorer::PanelsWidget *panelsWidget)
{
    panelsWidget->widget()->setMaximumWidth(900);
    panelsWidget->widget()->setMinimumWidth(0);
}

TextEditor::BaseTextEditorWidget *BarDescriptorEditorWidget::sourceWidget() const
{
    return m_xmlSourceWidget;
}

void BarDescriptorEditorWidget::setFilePath(const QString &filePath)
{
    Core::IDocument *doc = m_xmlSourceWidget->baseTextDocument();
    if (doc)
        doc->setFilePath(filePath);
}

void BarDescriptorEditorWidget::updateDocumentContent()
{
    ProjectExplorer::TaskHub::clearTasks(Constants::QNX_TASK_CATEGORY_BARDESCRIPTOR);
    QString errorMsg;
    int errorLine;

    disconnect(barDescriptorDocument(), SIGNAL(changed()), this, SLOT(updateSourceView()));
    bool result = barDescriptorDocument()->loadContent(m_xmlSourceWidget->toPlainText(), true, &errorMsg, &errorLine);
    connect(barDescriptorDocument(), SIGNAL(changed()), this, SLOT(updateSourceView()));

    if (!result) {
        ProjectExplorer::TaskHub::addTask(ProjectExplorer::Task::Error, errorMsg, Constants::QNX_TASK_CATEGORY_BARDESCRIPTOR,
                                          Utils::FileName::fromString(barDescriptorDocument()->filePath()), errorLine);
        ProjectExplorer::TaskHub::requestPopup();
    }
}

void BarDescriptorEditorWidget::updateSourceView()
{
    bool blocked = m_xmlSourceWidget->blockSignals(true);

    int line;
    int column;
    int position = m_xmlSourceWidget->position();
    m_xmlSourceWidget->convertPosition(position, &line, &column);

    m_xmlSourceWidget->setPlainText(barDescriptorDocument()->xmlSource());

    m_xmlSourceWidget->gotoLine(line, column);

    m_xmlSourceWidget->blockSignals(blocked);
}

BarDescriptorDocument *BarDescriptorEditorWidget::barDescriptorDocument() const
{
    return qobject_cast<BarDescriptorDocument*>(m_editor->document());
}
