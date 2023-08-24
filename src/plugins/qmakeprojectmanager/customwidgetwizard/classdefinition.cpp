// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "classdefinition.h"
#include "../qmakeprojectmanagertr.h"

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QTextEdit>

namespace QmakeProjectManager {
namespace Internal {

ClassDefinition::ClassDefinition(QWidget *parent) :
    QTabWidget(parent),
    m_domXmlChanged(false)
{
    using namespace Layouting;

    // "Sources" tab
    auto sourceTab = new QWidget;
    m_libraryRadio = new QRadioButton(Tr::tr("&Link library"));
    auto includeRadio = new QRadioButton(Tr::tr("Include pro&ject"));
    includeRadio->setChecked(true);
    m_skeletonCheck = new QCheckBox(Tr::tr("Create s&keleton"));
    m_widgetLibraryLabel = new QLabel(Tr::tr("Widget librar&y:"));
    m_widgetLibraryEdit = new QLineEdit;
    m_widgetProjectLabel = new QLabel(Tr::tr("Widget project &file:"));
    m_widgetProjectEdit = new QLineEdit;
    m_widgetHeaderEdit = new QLineEdit;
    m_widgetSourceLabel = new QLabel(Tr::tr("Widge&t source file:"));
    m_widgetSourceEdit = new QLineEdit;
    m_widgetBaseClassLabel = new QLabel(Tr::tr("Widget &base class:"));
    m_widgetBaseClassEdit = new QLineEdit("QWidget");
    m_pluginClassEdit = new QLineEdit;
    m_pluginHeaderEdit = new QLineEdit;
    m_pluginSourceEdit = new QLineEdit;
    m_iconPathChooser = new Utils::PathChooser;
    m_iconPathChooser->setExpectedKind(Utils::PathChooser::File);
    m_iconPathChooser->setHistoryCompleter("Qmake.Icon.History");
    m_iconPathChooser->setPromptDialogTitle(Tr::tr("Select Icon"));
    m_iconPathChooser->setPromptDialogFilter(Tr::tr("Icon files (*.png *.ico *.jpg *.xpm *.tif *.svg)"));
    Form {
        empty, Row { Column { m_libraryRadio, includeRadio }, m_skeletonCheck}, br,
        m_widgetLibraryLabel, m_widgetLibraryEdit, br,
        m_widgetProjectLabel, m_widgetProjectEdit, br,
        Tr::tr("Widget h&eader file:"), m_widgetHeaderEdit, br,
        m_widgetSourceLabel, m_widgetSourceEdit, br,
        m_widgetBaseClassLabel, m_widgetBaseClassEdit, br,
        Tr::tr("Plugin class &name:"), m_pluginClassEdit, br,
        Tr::tr("Plugin &header file:"), m_pluginHeaderEdit, br,
        Tr::tr("Plugin sou&rce file:"), m_pluginSourceEdit, br,
        Tr::tr("Icon file:"), m_iconPathChooser, br,
    }.attachTo(sourceTab);
    addTab(sourceTab, Tr::tr("&Sources"));

    // "Description" tab
    auto descriptionTab = new QWidget;
    m_groupEdit = new QLineEdit;
    m_tooltipEdit = new QLineEdit;
    m_whatsthisEdit = new QTextEdit;
    m_containerCheck = new QCheckBox(Tr::tr("The widget is a &container"));
    Form {
        Tr::tr("G&roup:"), m_groupEdit, br,
        Tr::tr("&Tooltip:"), m_tooltipEdit, br,
        Tr::tr("W&hat's this:"), m_whatsthisEdit, br,
        empty, m_containerCheck, br,
    }.attachTo(descriptionTab);
    addTab(descriptionTab, Tr::tr("&Description"));

    // "Property defaults" tab
    auto propertyDefaultsTab = new QWidget;
    auto domXmlLabel = new QLabel(Tr::tr("dom&XML:"));
    m_domXmlEdit = new QTextEdit;
    domXmlLabel->setBuddy(m_domXmlEdit);
    Column {
        domXmlLabel,
        m_domXmlEdit,
    }.attachTo(propertyDefaultsTab);
    addTab(propertyDefaultsTab, Tr::tr("Property defa&ults"));

    connect(m_libraryRadio, &QRadioButton::toggled, this, &ClassDefinition::enableButtons);
    connect(m_skeletonCheck, &QCheckBox::toggled, this, &ClassDefinition::enableButtons);
    connect(m_widgetLibraryEdit, &QLineEdit::textChanged,
            this, &ClassDefinition::widgetLibraryChanged);
    connect(m_widgetHeaderEdit, &QLineEdit::textChanged,
            this, &ClassDefinition::widgetHeaderChanged);
    connect(m_pluginClassEdit, &QLineEdit::textChanged,
            this, &ClassDefinition::pluginClassChanged);
    connect(m_pluginHeaderEdit, &QLineEdit::textChanged,
            this, &ClassDefinition::pluginHeaderChanged);
    connect(m_domXmlEdit, &QTextEdit::textChanged,
            this, [this] { m_domXmlChanged = true; });
}

void ClassDefinition::enableButtons()
{
    const bool enLib = m_libraryRadio->isChecked();
    m_widgetLibraryLabel->setEnabled(enLib);
    m_widgetLibraryEdit->setEnabled(enLib);

    const bool enSrc = m_skeletonCheck->isChecked();
    m_widgetSourceLabel->setEnabled(enSrc);
    m_widgetSourceEdit->setEnabled(enSrc);
    m_widgetBaseClassLabel->setEnabled(enSrc);
    m_widgetBaseClassEdit->setEnabled(enSrc);

    const bool enPrj = !enLib || enSrc;
    m_widgetProjectLabel->setEnabled(enPrj);
    m_widgetProjectEdit->setEnabled(enPrj);
    m_widgetProjectEdit->setText(
        QFileInfo(m_widgetProjectEdit->text()).completeBaseName() +
        (m_libraryRadio->isChecked() ? QLatin1String(".pro") : QLatin1String(".pri")));
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
    m_widgetLibraryEdit->setText(name.toLower());
    m_widgetHeaderEdit->setText(m_fileNamingParameters.headerFileName(name));
    m_pluginClassEdit->setText(name + QLatin1String("Plugin"));
    if (!m_domXmlChanged) {
        m_domXmlEdit->setText(xmlFromClassName(name));
        m_domXmlChanged = false;
    }
}

void ClassDefinition::widgetLibraryChanged(const QString &text)
{
    m_widgetProjectEdit->setText(text +
        (m_libraryRadio->isChecked() ? QLatin1String(".pro") : QLatin1String(".pri")));
}

void ClassDefinition::widgetHeaderChanged(const QString &text)
{
    m_widgetSourceEdit->setText(m_fileNamingParameters.headerToSourceFileName(text));
}

void ClassDefinition::pluginClassChanged(const QString &text)
{
    m_pluginHeaderEdit->setText(m_fileNamingParameters.headerFileName(text));
}

void ClassDefinition::pluginHeaderChanged(const QString &text)
{
    m_pluginSourceEdit->setText(m_fileNamingParameters.headerToSourceFileName(text));
}

PluginOptions::WidgetOptions ClassDefinition::widgetOptions(const QString &className) const
{
    PluginOptions::WidgetOptions wo;
    wo.createSkeleton = m_skeletonCheck->isChecked();
    wo.sourceType =
            m_libraryRadio->isChecked() ?
            PluginOptions::WidgetOptions::LinkLibrary :
            PluginOptions::WidgetOptions::IncludeProject;
    wo.widgetLibrary = m_widgetLibraryEdit->text();
    wo.widgetProjectFile = m_widgetProjectEdit->text();
    wo.widgetClassName = className;
    wo.widgetHeaderFile = m_widgetHeaderEdit->text();
    wo.widgetSourceFile = m_widgetSourceEdit->text();
    wo.widgetBaseClassName = m_widgetBaseClassEdit->text();
    wo.pluginClassName = m_pluginClassEdit->text();
    wo.pluginHeaderFile = m_pluginHeaderEdit->text();
    wo.pluginSourceFile = m_pluginSourceEdit->text();
    wo.iconFile = m_iconPathChooser->filePath().toString();
    wo.group = m_groupEdit->text();
    wo.toolTip = m_tooltipEdit->text();
    wo.whatsThis = m_whatsthisEdit->toPlainText();
    wo.isContainer = m_containerCheck->isChecked();
    wo.domXml = m_domXmlEdit->toPlainText();
    return wo;
}

}
}
