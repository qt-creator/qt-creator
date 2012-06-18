/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "unconfiguredsettingsoptionpage.h"
#include "qt4projectmanagerconstants.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtversionmanager.h>
#include <qt4projectmanager/qt4projectmanager.h>

#include <QCoreApplication>
#include <QIcon>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>
#include <QFormLayout>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

//////////////////////////
// UnConfiguredSettingsWidget
//////////////////////////

UnConfiguredSettingsWidget::UnConfiguredSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    QFormLayout *layout = new QFormLayout(this);

    QLabel *descriptionLabel = new QLabel;
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setText(tr("Qt Creator can open qmake projects without configuring them for building.\n"
                                 "The C++ and QML code models need a Qt version and tool chain to offer code completion.\n"));
    layout->addRow(descriptionLabel);

    m_qtVersionComboBox = new QComboBox;
    layout->addRow(tr("Qt Version:"), m_qtVersionComboBox);

    m_toolchainComboBox = new QComboBox;
    layout->addRow(tr("Tool Chain:"), m_toolchainComboBox);

    Qt4Manager *qt4Manager = ExtensionSystem::PluginManager::getObject<Qt4Manager>();
    Internal::UnConfiguredSettings ucs = qt4Manager->unconfiguredSettings();

    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    foreach (QtSupport::BaseQtVersion *version, vm->validVersions())
        m_qtVersionComboBox->addItem(version->displayName(), version->uniqueId());

    int index = ucs.version ? m_qtVersionComboBox->findData(ucs.version->uniqueId()) : 0;
    if (index == -1)
        index = 0;
    if (index < m_qtVersionComboBox->count())
        m_qtVersionComboBox->setCurrentIndex(index);


    ProjectExplorer::ToolChainManager *tm = ProjectExplorer::ToolChainManager::instance();
    foreach (ProjectExplorer::ToolChain *tc, tm->toolChains())
        m_toolchainComboBox->addItem(tc->displayName(), tc->id());

    index = ucs.toolchain ? m_toolchainComboBox->findData(ucs.toolchain->id()) : 0;
    if (index == -1)
        index = 0;
    if (index < m_toolchainComboBox->count())
        m_toolchainComboBox->setCurrentIndex(index);
}

void UnConfiguredSettingsWidget::apply()
{
    Qt4Manager *qt4Manager = ExtensionSystem::PluginManager::getObject<Qt4Manager>();
    Internal::UnConfiguredSettings ucs;

    int index = m_qtVersionComboBox->currentIndex();
    int qtVersionId = (index != -1) ? m_qtVersionComboBox->itemData(index).toInt() : -1;
    ucs.version = QtSupport::QtVersionManager::instance()->version(qtVersionId);
    index = m_toolchainComboBox->currentIndex();
    QString toolChainId = index != -1 ? m_toolchainComboBox->itemData(index).toString() : QString();
    ucs.toolchain = ProjectExplorer::ToolChainManager::instance()->findToolChain(toolChainId);
    qt4Manager->setUnconfiguredSettings(ucs);
}

bool UnConfiguredSettingsWidget::matches(const QString &searchKeyword)
{
    for (int i = 0; i < layout()->count(); ++i) {
        if (QLabel *l = qobject_cast<QLabel *>(layout()->itemAt(i)->widget()))
            if (l->text().contains(searchKeyword))
                return true;
    }
    return false;
}

//////////////////////////
// UnConfiguredSettingsOptionPage
//////////////////////////

UnConfiguredSettingsOptionPage::UnConfiguredSettingsOptionPage()
{
    setId(Constants::UNCONFIGURED_SETTINGS_PAGE_ID);
    setDisplayName(QCoreApplication::translate("Qt4ProjectManager", Constants::UNCONFIGURED_SETTINGS_PAGE_NAME));
    setCategory(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
        ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

bool UnConfiguredSettingsOptionPage::matches(const QString &searchKeyword) const
{
    return m_widget->matches(searchKeyword);
}

QWidget *UnConfiguredSettingsOptionPage::createPage(QWidget *parent)
{
    m_widget = new UnConfiguredSettingsWidget(parent);
    return m_widget;
}

void UnConfiguredSettingsOptionPage::apply()
{
    if (m_widget)
        m_widget->apply();
}

void UnConfiguredSettingsOptionPage::finish()
{

}
