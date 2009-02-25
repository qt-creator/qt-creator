/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "embeddedpropertiespage.h"
#include "qt4project.h"

#include <QFileInfo>
#include <QDir>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

///
/// EmbeddedPropertiesPanelFactory
///

bool EmbeddedPropertiesPanelFactory::supports(Project *project)
{
#ifdef Q_OS_WIN
    Qt4Project *pro = qobject_cast<Qt4Project *>(project);
    if (pro) {
        return true;
    }
#else
    Q_UNUSED(project);
#endif
    return false;
}

ProjectExplorer::PropertiesPanel *EmbeddedPropertiesPanelFactory::createPanel(
        ProjectExplorer::Project *project)
{
    return new EmbeddedPropertiesPanel(project);
}

///
/// EmbeddedPropertiesPanel
///

EmbeddedPropertiesPanel::EmbeddedPropertiesPanel(ProjectExplorer::Project *project)
    : PropertiesPanel(),
      m_widget(new EmbeddedPropertiesWidget(project))
{
}

EmbeddedPropertiesPanel::~EmbeddedPropertiesPanel()
{
    delete m_widget;
}

QString EmbeddedPropertiesPanel::name() const
{
    return tr("Embedded Linux");

}

QWidget *EmbeddedPropertiesPanel::widget()
{
    return m_widget;
}

///
/// EmbeddedPropertiesWidget
///

EmbeddedPropertiesWidget::EmbeddedPropertiesWidget(ProjectExplorer::Project *project)
    : QWidget()
{
    m_ui.setupUi(this);

#ifdef Q_OS_WIN
    m_ui.virtualBoxCheckbox->setChecked(project->value("useVBOX").toBool());

    // Find all skins
    QString skin = QFileInfo(project->value("VNCSkin").toString()).fileName();
    QStringList skins;

    QDir skinDir = QApplication::applicationDirPath();
    skinDir.cdUp();
    if (skinDir.cd("qtembeddedtools") && skinDir.cd("qsimplevnc")) {
        skins = skinDir.entryList( QDir::Dirs | QDir::NoDotAndDotDot );
    }
    m_ui.skinComboBox->clear();
    m_ui.skinComboBox->addItems(skins);
    if (!skin.isEmpty()) {
        int index = m_ui.skinComboBox->findText(skin);
        if (index != -1)
            m_ui.skinComboBox->setCurrentIndex(index);
    }
#else
    Q_UNUSED(project)
#endif
    //TODO readd finish code
    /*
    project->setValue("useVBOX", m_ui.virtualBoxCheckbox->isChecked());

    //Skin
    QDir skinDir = QApplication::applicationDirPath();
    skinDir.cdUp();
    skinDir.cd("qtembeddedtools");
    skinDir.cd("qsimplevnc");
    project->setValue("VNCSkin", skinDir.absolutePath() + "/" + m_ui.skinComboBox->currentText() + "/" + m_ui.skinComboBox->currentText());

    */
}

EmbeddedPropertiesWidget::~EmbeddedPropertiesWidget()
{
}

