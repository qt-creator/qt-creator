/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "classdefinition.h"

#include <QtCore/QFileInfo>
#include <QtGui/QFileDialog>

namespace Qt4ProjectManager {
namespace Internal {

ClassDefinition::ClassDefinition(QWidget *parent) :
    QTabWidget(parent),
    m_domXmlChanged(false)
{
    setupUi(this);
    iconPathChooser->setExpectedKind(Utils::PathChooser::File);
    iconPathChooser->setPromptDialogTitle(tr("Select Icon"));
    iconPathChooser->setPromptDialogFilter(tr("Icon files (*.png *.ico *.jpg *.xpm *.tif *.svg)"));
}

void ClassDefinition::on_libraryRadio_toggled()
{
    const bool enLib = libraryRadio->isChecked();
    widgetLibraryLabel->setEnabled(enLib);
    widgetLibraryEdit->setEnabled(enLib);

    const bool enSrc = skeletonCheck->isChecked();
    widgetSourceLabel->setEnabled(enSrc);
    widgetSourceEdit->setEnabled(enSrc);
    widgetBaseClassLabel->setEnabled(enSrc);
    widgetBaseClassEdit->setEnabled(enSrc);

    const bool enPrj = !enLib || enSrc;
    widgetProjectLabel->setEnabled(enPrj);
    widgetProjectEdit->setEnabled(enPrj);
    widgetProjectEdit->setText(
        QFileInfo(widgetProjectEdit->text()).completeBaseName() +
        (libraryRadio->isChecked() ? QLatin1String(".pro") : QLatin1String(".pri")));
}

void ClassDefinition::on_skeletonCheck_toggled()
{
    on_libraryRadio_toggled();
}

static inline QString xmlFromClassName(const QString &name)
{
    QString rc = QLatin1String("<widget class=\"");
    rc += name;
    rc += QLatin1String("\" name=\"");
    if (!name.isEmpty()) {
        rc += name.left(1).toLower();
        if (name.size() > 1)
            rc += name.mid(1);
    }
    rc += QLatin1String("\">\n</widget>\n");
    return rc;
}

void ClassDefinition::setClassName(const QString &name)
{
    widgetLibraryEdit->setText(name.toLower());
    widgetHeaderEdit->setText(m_fileNamingParameters.headerFileName(name));
    pluginClassEdit->setText(name + QLatin1String("Plugin"));
    if (!m_domXmlChanged) {
        domXmlEdit->setText(xmlFromClassName(name));
        m_domXmlChanged = false;
    }
}

void ClassDefinition::on_widgetLibraryEdit_textChanged()
{
    widgetProjectEdit->setText(
        widgetLibraryEdit->text() +
        (libraryRadio->isChecked() ? QLatin1String(".pro") : QLatin1String(".pri")));
}

void ClassDefinition::on_widgetHeaderEdit_textChanged()
{
    widgetSourceEdit->setText(m_fileNamingParameters.headerToSourceFileName(widgetHeaderEdit->text()));
}

void ClassDefinition::on_pluginClassEdit_textChanged()
{
    pluginHeaderEdit->setText(m_fileNamingParameters.headerFileName(pluginClassEdit->text()));
}

void ClassDefinition::on_pluginHeaderEdit_textChanged()
{
    pluginSourceEdit->setText(m_fileNamingParameters.headerToSourceFileName(pluginHeaderEdit->text()));
}

void ClassDefinition::on_domXmlEdit_textChanged()
{
    m_domXmlChanged = true;
}

PluginOptions::WidgetOptions ClassDefinition::widgetOptions(const QString &className) const
{
    PluginOptions::WidgetOptions wo;
    wo.createSkeleton = skeletonCheck->isChecked();
    wo.sourceType =
            libraryRadio->isChecked() ?
            PluginOptions::WidgetOptions::LinkLibrary :
            PluginOptions::WidgetOptions::IncludeProject;
    wo.widgetLibrary = widgetLibraryEdit->text();
    wo.widgetProjectFile = widgetProjectEdit->text();
    wo.widgetClassName = className;
    wo.widgetHeaderFile = widgetHeaderEdit->text();
    wo.widgetSourceFile = widgetSourceEdit->text();
    wo.widgetBaseClassName = widgetBaseClassEdit->text();
    wo.pluginClassName = pluginClassEdit->text();
    wo.pluginHeaderFile = pluginHeaderEdit->text();
    wo.pluginSourceFile = pluginSourceEdit->text();
    wo.iconFile = iconPathChooser->path();
    wo.group = groupEdit->text();
    wo.toolTip = tooltipEdit->text();
    wo.whatsThis = whatsthisEdit->toPlainText();
    wo.isContainer = containerCheck->isChecked();
    wo.domXml = domXmlEdit->toPlainText();
    return wo;
}

}
}
