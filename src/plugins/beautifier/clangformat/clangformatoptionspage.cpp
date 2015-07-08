/**************************************************************************
**
** Copyright (C) 2015 Lorenz Haas
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

#include "clangformatoptionspage.h"
#include "ui_clangformatoptionspage.h"

#include "clangformatconstants.h"
#include "clangformatsettings.h"

#include "../beautifierconstants.h"
#include "../beautifierplugin.h"

#include <coreplugin/icore.h>

#include <QTextStream>

namespace Beautifier {
namespace Internal {
namespace ClangFormat {

ClangFormatOptionsPageWidget::ClangFormatOptionsPageWidget(ClangFormatSettings *settings,
                                                           QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ClangFormatOptionsPage)
    , m_settings(settings)
{
    ui->setupUi(this);
    ui->options->setEnabled(false);
    ui->predefinedStyle->addItems(m_settings->predefinedStyles());
    ui->command->setExpectedKind(Utils::PathChooser::ExistingCommand);
    ui->command->setPromptDialogTitle(
                BeautifierPlugin::msgCommandPromptDialogTitle(QLatin1String("Clang Format")));
    connect(ui->command, &Utils::PathChooser::validChanged, ui->options, &QWidget::setEnabled);
    ui->configurations->setSettings(m_settings);
}

ClangFormatOptionsPageWidget::~ClangFormatOptionsPageWidget()
{
    delete ui;
}

void ClangFormatOptionsPageWidget::restore()
{
    ui->command->setPath(m_settings->command());
    const int textIndex = ui->predefinedStyle->findText(m_settings->predefinedStyle());
    if (textIndex != -1)
        ui->predefinedStyle->setCurrentIndex(textIndex);
    ui->formatEntireFileFallback->setChecked(m_settings->formatEntireFileFallback());
    ui->configurations->setSettings(m_settings);
    ui->configurations->setCurrentConfiguration(m_settings->customStyle());

    if (m_settings->usePredefinedStyle())
        ui->usePredefinedStyle->setChecked(true);
    else
        ui->useCustomizedStyle->setChecked(true);
}

void ClangFormatOptionsPageWidget::apply()
{
    m_settings->setCommand(ui->command->path());
    m_settings->setUsePredefinedStyle(ui->usePredefinedStyle->isChecked());
    m_settings->setPredefinedStyle(ui->predefinedStyle->currentText());
    m_settings->setCustomStyle(ui->configurations->currentConfiguration());
    m_settings->setFormatEntireFileFallback(ui->formatEntireFileFallback->isChecked());
    m_settings->save();
}

/* ---------------------------------------------------------------------------------------------- */

ClangFormatOptionsPage::ClangFormatOptionsPage(ClangFormatSettings *settings, QObject *parent) :
    IOptionsPage(parent),
    m_widget(0),
    m_settings(settings)
{
    setId(Constants::ClangFormat::OPTION_ID);
    setDisplayName(tr("Clang Format"));
    setCategory(Constants::OPTION_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Beautifier", Constants::OPTION_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::OPTION_CATEGORY_ICON));
}

QWidget *ClangFormatOptionsPage::widget()
{
    m_settings->read();

    if (!m_widget)
        m_widget = new ClangFormatOptionsPageWidget(m_settings);
    m_widget->restore();

    return m_widget;
}

void ClangFormatOptionsPage::apply()
{
    if (m_widget)
        m_widget->apply();
}

void ClangFormatOptionsPage::finish()
{
}

} // namespace ClangFormat
} // namespace Internal
} // namespace Beautifier
