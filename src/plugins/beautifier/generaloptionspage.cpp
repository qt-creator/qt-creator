/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include "generaloptionspage.h"
#include "ui_generaloptionspage.h"

#include "beautifierconstants.h"
#include "generalsettings.h"

#include <coreplugin/icore.h>

#include <QTextStream>

namespace Beautifier {
namespace Internal {

GeneralOptionsPageWidget::GeneralOptionsPageWidget(const QSharedPointer<GeneralSettings> &settings,
                                                   const QStringList &toolIds, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeneralOptionsPage),
    m_settings(settings)
{
    ui->setupUi(this);
    ui->autoFormatTool->addItems(toolIds);
    restore();
}

GeneralOptionsPageWidget::~GeneralOptionsPageWidget()
{
    delete ui;
}

void GeneralOptionsPageWidget::restore()
{
    ui->autoFormat->setChecked(m_settings->autoFormatOnSave());
    const int index = ui->autoFormatTool->findText(m_settings->autoFormatTool());
    ui->autoFormatTool->setCurrentIndex(qMax(index, 0));
    ui->autoFormatMime->setText(m_settings->autoFormatMimeAsString());
    ui->autoFormatOnlyCurrentProject->setChecked(m_settings->autoFormatOnlyCurrentProject());
}

void GeneralOptionsPageWidget::apply(bool *autoFormatChanged)
{
    if (autoFormatChanged)
        *autoFormatChanged = m_settings->autoFormatOnSave() != ui->autoFormat->isChecked();

    m_settings->setAutoFormatOnSave(ui->autoFormat->isChecked());
    m_settings->setAutoFormatTool(ui->autoFormatTool->currentText());
    m_settings->setAutoFormatMime(ui->autoFormatMime->text());
    m_settings->setAutoFormatOnlyCurrentProject(ui->autoFormatOnlyCurrentProject->isChecked());
    m_settings->save();
}

GeneralOptionsPage::GeneralOptionsPage(const QSharedPointer<GeneralSettings> &settings,
                                       const QStringList &toolIds, QObject *parent) :
    IOptionsPage(parent),
    m_settings(settings),
    m_toolIds(toolIds)
{
    setId(Constants::OPTION_GENERAL_ID);
    setDisplayName(tr("General"));
    setCategory(Constants::OPTION_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Beautifier", Constants::OPTION_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(Constants::OPTION_CATEGORY_ICON));
}

QWidget *GeneralOptionsPage::widget()
{
    m_settings->read();

    if (!m_widget)
        m_widget = new GeneralOptionsPageWidget(m_settings, m_toolIds);
    m_widget->restore();

    return m_widget;
}

void GeneralOptionsPage::apply()
{
    if (m_widget) {
        bool autoFormat = false;
        m_widget->apply(&autoFormat);
        if (autoFormat)
            emit autoFormatChanged();
    }
}

void GeneralOptionsPage::finish()
{
}

} // namespace Internal
} // namespace Beautifier
