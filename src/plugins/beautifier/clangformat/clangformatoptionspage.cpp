/**************************************************************************
**
** Copyright (c) 2014 Lorenz Haas
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

#include "clangformatoptionspage.h"
#include "ui_clangformatoptionspage.h"

#include "clangformatconstants.h"
#include "clangformatsettings.h"

#include "../beautifierconstants.h"

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
    ui->command->setPromptDialogTitle(tr("ClangFormat Command"));
    connect(ui->command, SIGNAL(validChanged(bool)), ui->options, SLOT(setEnabled(bool)));
    ui->configurations->setSettings(m_settings);
}

ClangFormatOptionsPageWidget::~ClangFormatOptionsPageWidget()
{
    delete ui;
}

QString ClangFormatOptionsPageWidget::searchKeywords() const
{
    QString keywords;
    const QLatin1Char sep(' ');
    QTextStream(&keywords) << sep << ui->configuration->title()
                           << sep << ui->commandLabel->text()
                           << sep << ui->options->title()
                           << sep << ui->usePredefinedStyle->text()
                           << sep << ui->useCustomizedStyle->text()
                           << sep << ui->formatEntireFileFallback->text();
    keywords.remove(QLatin1Char('&'));
    return keywords;
}

void ClangFormatOptionsPageWidget::restore()
{
    ui->command->setPath(m_settings->command());
    int textIndex = ui->predefinedStyle->findText(m_settings->predefinedStyle());
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
    m_settings(settings),
    m_searchKeywords()
{
    setId(Constants::ClangFormat::OPTION_ID);
    setDisplayName(tr("ClangFormat"));
    setCategory(Constants::OPTION_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Beautifier", Constants::OPTION_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::OPTION_CATEGORY_ICON));
}

QWidget *ClangFormatOptionsPage::widget()
{
    m_settings->read();

    if (!m_widget) {
        m_widget = new ClangFormatOptionsPageWidget(m_settings);
        if (m_searchKeywords.isEmpty())
            m_searchKeywords = m_widget->searchKeywords();
    }
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

bool ClangFormatOptionsPage::matches(const QString &searchKeyWord) const
{
    return m_searchKeywords.contains(searchKeyWord, Qt::CaseInsensitive);
}

} // namespace ClangFormat
} // namespace Internal
} // namespace Beautifier
