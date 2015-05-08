/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppcodemodelsettingspage.h"
#include "cpptoolsconstants.h"
#include "ui_cppcodemodelsettingspage.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>

#include <QTextStream>

using namespace CppTools;
using namespace CppTools::Internal;

CppCodeModelSettingsWidget::CppCodeModelSettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::CppCodeModelSettingsPage)
{
    m_ui->setupUi(this);

    m_ui->theGroupBox->setVisible(true);
}

CppCodeModelSettingsWidget::~CppCodeModelSettingsWidget()
{
    delete m_ui;
}

void CppCodeModelSettingsWidget::setSettings(const QSharedPointer<CppCodeModelSettings> &s)
{
    m_settings = s;

    applyToWidget(m_ui->cChooser, QLatin1String(Constants::C_SOURCE_MIMETYPE));
    applyToWidget(m_ui->cppChooser, QLatin1String(Constants::CPP_SOURCE_MIMETYPE));
    applyToWidget(m_ui->objcChooser, QLatin1String(Constants::OBJECTIVE_C_SOURCE_MIMETYPE));
    applyToWidget(m_ui->objcppChooser, QLatin1String(Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE));
    applyToWidget(m_ui->hChooser, QLatin1String(Constants::C_HEADER_MIMETYPE));

    m_ui->ignorePCHCheckBox->setChecked(s->pchUsage() == CppCodeModelSettings::PchUse_None);
}

void CppCodeModelSettingsWidget::applyToWidget(QComboBox *chooser, const QString &mimeType) const
{
    chooser->clear();

    QStringList names = m_settings->availableModelManagerSupportProvidersByName().keys();
    Utils::sort(names);
    foreach (const QString &name, names) {
        const QString &id = m_settings->availableModelManagerSupportProvidersByName()[name];
        chooser->addItem(name, id);
        if (id == m_settings->modelManagerSupportIdForMimeType(mimeType))
            chooser->setCurrentIndex(chooser->count() - 1);
    }
    chooser->setEnabled(names.size() > 1);
}

void CppCodeModelSettingsWidget::applyToSettings() const
{
    bool changed = false;
    changed |= applyToSettings(m_ui->cChooser, QLatin1String(Constants::C_SOURCE_MIMETYPE));
    changed |= applyToSettings(m_ui->cppChooser, QLatin1String(Constants::CPP_SOURCE_MIMETYPE));
    changed |= applyToSettings(m_ui->objcChooser,
                               QLatin1String(Constants::OBJECTIVE_C_SOURCE_MIMETYPE));
    changed |= applyToSettings(m_ui->objcppChooser,
                               QLatin1String(Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE));
    changed |= applyToSettings(m_ui->hChooser,
                               QLatin1String(Constants::C_HEADER_MIMETYPE));

    if (m_ui->ignorePCHCheckBox->isChecked() !=
            (m_settings->pchUsage() == CppCodeModelSettings::PchUse_None)) {
        m_settings->setPCHUsage(
                   m_ui->ignorePCHCheckBox->isChecked() ? CppCodeModelSettings::PchUse_None
                                                        : CppCodeModelSettings::PchUse_BuildSystem);
        changed = true;
    }

    if (changed)
        m_settings->toSettings(Core::ICore::settings());
}

bool CppCodeModelSettingsWidget::applyToSettings(QComboBox *chooser, const QString &mimeType) const
{
    QString newId = chooser->itemData(chooser->currentIndex()).toString();
    QString currentId = m_settings->modelManagerSupportIdForMimeType(mimeType);
    if (newId == currentId)
        return false;

    m_settings->setModelManagerSupportIdForMimeType(mimeType, newId);
    return true;
}

CppCodeModelSettingsPage::CppCodeModelSettingsPage(QSharedPointer<CppCodeModelSettings> &settings,
                                                   QObject *parent)
    : Core::IOptionsPage(parent)
    , m_settings(settings)
{
    setId(Constants::CPP_CODE_MODEL_SETTINGS_ID);
    setDisplayName(QCoreApplication::translate("CppTools",Constants::CPP_CODE_MODEL_SETTINGS_NAME));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("CppTools",Constants::CPP_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::SETTINGS_CATEGORY_CPP_ICON));
}

QWidget *CppCodeModelSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new CppCodeModelSettingsWidget;
        m_widget->setSettings(m_settings);
    }
    return m_widget;
}

void CppCodeModelSettingsPage::apply()
{
    if (m_widget)
        m_widget->applyToSettings();
}

void CppCodeModelSettingsPage::finish()
{
    delete m_widget;
}
