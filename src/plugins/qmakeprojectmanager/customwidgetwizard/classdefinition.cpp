/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "classdefinition.h"

#include <QFileInfo>

namespace QmakeProjectManager {
namespace Internal {

ClassDefinition::ClassDefinition(QWidget *parent) :
    QTabWidget(parent),
    m_domXmlChanged(false)
{
    m_ui.setupUi(this);
    m_ui.iconPathChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui.iconPathChooser->setHistoryCompleter(QLatin1String("Qmake.Icon.History"));
    m_ui.iconPathChooser->setPromptDialogTitle(tr("Select Icon"));
    m_ui.iconPathChooser->setPromptDialogFilter(tr("Icon files (*.png *.ico *.jpg *.xpm *.tif *.svg)"));

    connect(m_ui.libraryRadio, &QRadioButton::toggled, this, &ClassDefinition::enableButtons);
    connect(m_ui.skeletonCheck, &QCheckBox::toggled, this, &ClassDefinition::enableButtons);
    connect(m_ui.widgetLibraryEdit, &QLineEdit::textChanged,
            this, &ClassDefinition::widgetLibraryChanged);
    connect(m_ui.widgetHeaderEdit, &QLineEdit::textChanged,
            this, &ClassDefinition::widgetHeaderChanged);
    connect(m_ui.pluginClassEdit, &QLineEdit::textChanged,
            this, &ClassDefinition::pluginClassChanged);
    connect(m_ui.pluginHeaderEdit, &QLineEdit::textChanged,
            this, &ClassDefinition::pluginHeaderChanged);
    connect(m_ui.domXmlEdit, &QTextEdit::textChanged,
            this, [this] { m_domXmlChanged = true; });
}

void ClassDefinition::enableButtons()
{
    const bool enLib = m_ui.libraryRadio->isChecked();
    m_ui.widgetLibraryLabel->setEnabled(enLib);
    m_ui.widgetLibraryEdit->setEnabled(enLib);

    const bool enSrc = m_ui.skeletonCheck->isChecked();
    m_ui.widgetSourceLabel->setEnabled(enSrc);
    m_ui.widgetSourceEdit->setEnabled(enSrc);
    m_ui.widgetBaseClassLabel->setEnabled(enSrc);
    m_ui.widgetBaseClassEdit->setEnabled(enSrc);

    const bool enPrj = !enLib || enSrc;
    m_ui.widgetProjectLabel->setEnabled(enPrj);
    m_ui.widgetProjectEdit->setEnabled(enPrj);
    m_ui.widgetProjectEdit->setText(
        QFileInfo(m_ui.widgetProjectEdit->text()).completeBaseName() +
        (m_ui.libraryRadio->isChecked() ? QLatin1String(".pro") : QLatin1String(".pri")));
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
    m_ui.widgetLibraryEdit->setText(name.toLower());
    m_ui.widgetHeaderEdit->setText(m_fileNamingParameters.headerFileName(name));
    m_ui.pluginClassEdit->setText(name + QLatin1String("Plugin"));
    if (!m_domXmlChanged) {
        m_ui.domXmlEdit->setText(xmlFromClassName(name));
        m_domXmlChanged = false;
    }
}

void ClassDefinition::widgetLibraryChanged(const QString &text)
{
    m_ui.widgetProjectEdit->setText(text +
        (m_ui.libraryRadio->isChecked() ? QLatin1String(".pro") : QLatin1String(".pri")));
}

void ClassDefinition::widgetHeaderChanged(const QString &text)
{
    m_ui.widgetSourceEdit->setText(m_fileNamingParameters.headerToSourceFileName(text));
}

void ClassDefinition::pluginClassChanged(const QString &text)
{
    m_ui.pluginHeaderEdit->setText(m_fileNamingParameters.headerFileName(text));
}

void ClassDefinition::pluginHeaderChanged(const QString &text)
{
    m_ui.pluginSourceEdit->setText(m_fileNamingParameters.headerToSourceFileName(text));
}

PluginOptions::WidgetOptions ClassDefinition::widgetOptions(const QString &className) const
{
    PluginOptions::WidgetOptions wo;
    wo.createSkeleton = m_ui.skeletonCheck->isChecked();
    wo.sourceType =
            m_ui.libraryRadio->isChecked() ?
            PluginOptions::WidgetOptions::LinkLibrary :
            PluginOptions::WidgetOptions::IncludeProject;
    wo.widgetLibrary = m_ui.widgetLibraryEdit->text();
    wo.widgetProjectFile = m_ui.widgetProjectEdit->text();
    wo.widgetClassName = className;
    wo.widgetHeaderFile = m_ui.widgetHeaderEdit->text();
    wo.widgetSourceFile = m_ui.widgetSourceEdit->text();
    wo.widgetBaseClassName = m_ui.widgetBaseClassEdit->text();
    wo.pluginClassName = m_ui.pluginClassEdit->text();
    wo.pluginHeaderFile = m_ui.pluginHeaderEdit->text();
    wo.pluginSourceFile = m_ui.pluginSourceEdit->text();
    wo.iconFile = m_ui.iconPathChooser->filePath().toString();
    wo.group = m_ui.groupEdit->text();
    wo.toolTip = m_ui.tooltipEdit->text();
    wo.whatsThis = m_ui.whatsthisEdit->toPlainText();
    wo.isContainer = m_ui.containerCheck->isChecked();
    wo.domXml = m_ui.domXmlEdit->toPlainText();
    return wo;
}

}
}
