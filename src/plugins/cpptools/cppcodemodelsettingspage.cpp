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

    m_ui->clangSettingsGroupBox->setVisible(true);
    connect(m_ui->clangOptionsResetButton, &QPushButton::clicked, [this]() {
        const QString options = m_settings->defaultExtraClangOptions().join(QLatin1Char('\n'));
        m_ui->clangOptionsToAppendTextEdit->document()->setPlainText(options);
    });
}

CppCodeModelSettingsWidget::~CppCodeModelSettingsWidget()
{
    delete m_ui;
}

void CppCodeModelSettingsWidget::setSettings(const QSharedPointer<CppCodeModelSettings> &s)
{
    m_settings = s;

    setupClangCodeModelWidgets();
    m_ui->ignorePCHCheckBox->setChecked(s->pchUsage() == CppCodeModelSettings::PchUse_None);
}

void CppCodeModelSettingsWidget::applyToSettings() const
{
    bool changed = false;

    if (applyClangCodeModelWidgetsToSettings())
        changed = true;

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

static bool isClangCodeModelActive(const CppCodeModelSettings &settings)
{
    const QString currentCodeModelId
        = settings.modelManagerSupportIdForMimeType(QLatin1String(Constants::CPP_SOURCE_MIMETYPE));
    return currentCodeModelId != settings.defaultId();
}

void CppCodeModelSettingsWidget::setupClangCodeModelWidgets() const
{
    bool isClangActive = false;
    const bool isClangAvailable = m_settings->availableModelManagerSupportProvidersByName().size() > 1;
    if (isClangAvailable)
        isClangActive = isClangCodeModelActive(*m_settings.data());

    m_ui->activateClangCodeModelPluginHint->setVisible(!isClangAvailable);
    m_ui->clangSettingsGroupBox->setEnabled(isClangAvailable);
    m_ui->clangSettingsGroupBox->setChecked(isClangActive);

    const QString extraClangOptions = m_settings->extraClangOptions().join(QLatin1Char('\n'));
    m_ui->clangOptionsToAppendTextEdit->document()->setPlainText(extraClangOptions);
}

bool CppCodeModelSettingsWidget::applyClangCodeModelWidgetsToSettings() const
{
    // Once the underlying settings are not mime type based anymore, simplify here.
    // Until then, ensure that the settings are set uniformly for all the mime types
    // to avoid surprises.

    const QString activeCodeModelId = m_ui->clangSettingsGroupBox->isChecked()
            ? QLatin1String("ClangCodeMode.ClangCodeMode")
            : QLatin1String("CppTools.BuiltinCodeModel");

    foreach (const QString &mimeType, m_settings->supportedMimeTypes())
        m_settings->setModelManagerSupportIdForMimeType(mimeType, activeCodeModelId);

    const QString clangOptionsText = m_ui->clangOptionsToAppendTextEdit->document()->toPlainText();
    const QStringList extraClangOptions = clangOptionsText.split(QLatin1Char('\n'),
                                                                 QString::SkipEmptyParts);
    m_settings->setExtraClangOptions(extraClangOptions);

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
