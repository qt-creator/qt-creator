/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
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

#include <projectexplorer/iprojectproperties.h>
#include <projectexplorer/projectwindow.h>
#include <texteditor/plaintexteditor.h>

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorEditorWidget::BarDescriptorEditorWidget(QWidget *parent)
    : QStackedWidget(parent)
    , m_editor(0)
    , m_dirty(false)
{
    initGeneralPage();
    initApplicationPage();
    initAssetsPage();
    initSourcePage();

    setCurrentIndex(0);
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

    connect(m_entryPointWidget, SIGNAL(changed()), this, SLOT(setDirty()));
    connect(m_packageInformationWidget, SIGNAL(changed()), this, SLOT(setDirty()));
    connect(m_authorInformationWidget, SIGNAL(changed()), this, SLOT(setDirty()));
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

    connect(m_generalWidget, SIGNAL(changed()), this, SLOT(setDirty()));
    connect(m_permissionsWidget, SIGNAL(changed()), this, SLOT(setDirty()));
    connect(m_environmentWidget, SIGNAL(changed()), this, SLOT(setDirty()));
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

    connect(m_assetsWidget, SIGNAL(changed()), this, SLOT(setDirty()));

    m_entryPointWidget->setAssetsModel(m_assetsWidget->assetsModel());
    connect(m_entryPointWidget, SIGNAL(imageAdded(QString)), m_assetsWidget, SLOT(addAsset(QString)));
    connect(m_entryPointWidget, SIGNAL(imageRemoved(QString)), m_assetsWidget, SLOT(removeAsset(QString)));
}

void BarDescriptorEditorWidget::initSourcePage()
{
    m_xmlSourceWidget = new TextEditor::PlainTextEditorWidget(this);
    addWidget(m_xmlSourceWidget);

    m_xmlSourceWidget->configure(QLatin1String(Constants::QNX_BAR_DESCRIPTOR_MIME_TYPE));
    connect(m_xmlSourceWidget, SIGNAL(textChanged()), this, SLOT(setDirty()));
}

void BarDescriptorEditorWidget::initPanelSize(ProjectExplorer::PanelsWidget *panelsWidget)
{
    panelsWidget->widget()->setMaximumWidth(900);
    panelsWidget->widget()->setMinimumWidth(0);
}

Core::IEditor *BarDescriptorEditorWidget::editor() const
{
    if (!m_editor) {
        m_editor = const_cast<BarDescriptorEditorWidget *>(this)->createEditor();
        connect(this, SIGNAL(changed()), m_editor, SIGNAL(changed()));
    }

    return m_editor;
}

BarDescriptorEditorPackageInformationWidget *BarDescriptorEditorWidget::packageInformationWidget() const
{
    return m_packageInformationWidget;
}

BarDescriptorEditorAuthorInformationWidget *BarDescriptorEditorWidget::authorInformationWidget() const
{
    return m_authorInformationWidget;
}

BarDescriptorEditorEntryPointWidget *BarDescriptorEditorWidget::entryPointWidget() const
{
    return m_entryPointWidget;
}

BarDescriptorEditorGeneralWidget *BarDescriptorEditorWidget::generalWidget() const
{
    return m_generalWidget;
}

BarDescriptorEditorPermissionsWidget *BarDescriptorEditorWidget::permissionsWidget() const
{
    return m_permissionsWidget;
}

BarDescriptorEditorEnvironmentWidget *BarDescriptorEditorWidget::environmentWidget() const
{
    return m_environmentWidget;
}

BarDescriptorEditorAssetsWidget *BarDescriptorEditorWidget::assetsWidget() const
{
    return m_assetsWidget;
}

QString BarDescriptorEditorWidget::xmlSource() const
{
    return m_xmlSourceWidget->toPlainText();
}

void BarDescriptorEditorWidget::setXmlSource(const QString &xmlSource)
{
    bool blocked = m_xmlSourceWidget->blockSignals(true);
    m_xmlSourceWidget->setPlainText(xmlSource);
    m_xmlSourceWidget->blockSignals(blocked);
}

bool BarDescriptorEditorWidget::isDirty() const
{
    return m_dirty;
}

void BarDescriptorEditorWidget::clear()
{
    m_entryPointWidget->clear();
    m_packageInformationWidget->clear();
    m_authorInformationWidget->clear();

    m_generalWidget->clear();
    m_permissionsWidget->clear();
    m_environmentWidget->clear();

    m_assetsWidget->clear();

    bool blocked = m_xmlSourceWidget->blockSignals(true);
    m_xmlSourceWidget->clear();
    m_xmlSourceWidget->blockSignals(blocked);
}

void BarDescriptorEditorWidget::setDirty(bool dirty)
{
    m_dirty = dirty;
    emit changed();
}

BarDescriptorEditor *BarDescriptorEditorWidget::createEditor()
{
    return new BarDescriptorEditor(this);
}
