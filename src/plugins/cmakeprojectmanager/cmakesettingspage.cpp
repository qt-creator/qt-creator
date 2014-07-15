/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include "cmakesettingspage.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/icore.h>
#include <utils/environment.h>

#include <QSettings>
#include <QGroupBox>
#include <QSpacerItem>
#include <QFormLayout>
#include <QBoxLayout>
#include <QCheckBox>

namespace CMakeProjectManager {
namespace Internal {

/////
// CMakeSettingsPage
////


CMakeSettingsPage::CMakeSettingsPage()
    :  m_pathchooser(0), m_preferNinja(0)
{
    setId("Z.CMake");
    setDisplayName(tr("CMake"));
    setCategory(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
       ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));

    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String("CMakeSettings"));
    m_cmakeValidatorForUser.setCMakeExecutable(settings->value(QLatin1String("cmakeExecutable")).toString());
    settings->endGroup();

    m_cmakeValidatorForSystem.setCMakeExecutable(findCmakeExecutable());
}

bool CMakeSettingsPage::isCMakeExecutableValid() const
{
    if (m_cmakeValidatorForUser.isValid())
        return true;

    return m_cmakeValidatorForSystem.isValid();
}

CMakeSettingsPage::~CMakeSettingsPage()
{
    m_cmakeValidatorForUser.cancel();
    m_cmakeValidatorForSystem.cancel();
}

QString CMakeSettingsPage::findCmakeExecutable() const
{
    return Utils::Environment::systemEnvironment().searchInPath(QLatin1String("cmake")).toString();
}

QWidget *CMakeSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new QWidget;
        QFormLayout *formLayout = new QFormLayout(m_widget);
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        m_pathchooser = new Utils::PathChooser;
        m_pathchooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
        m_pathchooser->setHistoryCompleter(QLatin1String("Cmake.Command.History"));
        formLayout->addRow(tr("Executable:"), m_pathchooser);
        formLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));

        m_preferNinja = new QCheckBox(tr("Prefer Ninja generator (CMake 2.8.9 or higher required)"));
        formLayout->addRow(m_preferNinja);
    }
    m_pathchooser->setPath(m_cmakeValidatorForUser.cmakeExecutable());
    m_preferNinja->setChecked(preferNinja());
    return m_widget;
}

void CMakeSettingsPage::saveSettings() const
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String("CMakeSettings"));
    settings->setValue(QLatin1String("cmakeExecutable"), m_cmakeValidatorForUser.cmakeExecutable());
    settings->setValue(QLatin1String("preferNinja"), m_preferNinja->isChecked());
    settings->endGroup();
}

void CMakeSettingsPage::apply()
{
    if (!m_pathchooser) // page was never shown
        return;
    if (m_cmakeValidatorForUser.cmakeExecutable() != m_pathchooser->path())
        m_cmakeValidatorForUser.setCMakeExecutable(m_pathchooser->path());
    saveSettings();
}

void CMakeSettingsPage::finish()
{
    delete m_widget;
}

QString CMakeSettingsPage::cmakeExecutable() const
{
    if (!isCMakeExecutableValid())
        return QString();

    if (m_cmakeValidatorForUser.isValid())
        return m_cmakeValidatorForUser.cmakeExecutable();
    if (m_cmakeValidatorForSystem.isValid())
        return m_cmakeValidatorForSystem.cmakeExecutable();
    return QString();
}

void CMakeSettingsPage::setCMakeExecutable(const QString &executable)
{
    if (m_cmakeValidatorForUser.cmakeExecutable() == executable)
        return;
    m_cmakeValidatorForUser.setCMakeExecutable(executable);
}

bool CMakeSettingsPage::hasCodeBlocksMsvcGenerator() const
{
    if (m_cmakeValidatorForUser.isValid())
        return m_cmakeValidatorForUser.hasCodeBlocksMsvcGenerator();
    if (m_cmakeValidatorForSystem.isValid())
        return m_cmakeValidatorForSystem.hasCodeBlocksMsvcGenerator();
    return false;
}

bool CMakeSettingsPage::hasCodeBlocksNinjaGenerator() const
{
    if (m_cmakeValidatorForUser.isValid())
        return m_cmakeValidatorForUser.hasCodeBlocksNinjaGenerator();
    if (m_cmakeValidatorForSystem.isValid())
        return m_cmakeValidatorForSystem.hasCodeBlocksNinjaGenerator();
    return false;
}

bool CMakeSettingsPage::preferNinja() const
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String("CMakeSettings"));
    const bool r = settings->value(QLatin1String("preferNinja"), false).toBool();
    settings->endGroup();
    return r;
}

TextEditor::Keywords CMakeSettingsPage::keywords()
{
    if (m_cmakeValidatorForUser.isValid())
        return m_cmakeValidatorForUser.keywords();

    if (m_cmakeValidatorForSystem.isValid())
        return m_cmakeValidatorForSystem.keywords();

    return TextEditor::Keywords(QStringList(), QStringList(), QMap<QString, QStringList>());
}

} // namespace Internal
} // namespace CMakeProjectManager
