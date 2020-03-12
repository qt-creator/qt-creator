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

namespace Beautifier {
namespace Internal {

class GeneralOptionsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Beautifier::Internal::GeneralOptionsPageWidget)

public:
    explicit GeneralOptionsPageWidget(const QStringList &toolIds);

private:
    void apply() final;

    Ui::GeneralOptionsPage ui;
};

GeneralOptionsPageWidget::GeneralOptionsPageWidget(const QStringList &toolIds)
{
    ui.setupUi(this);
    ui.autoFormatTool->addItems(toolIds);

    auto settings = GeneralSettings::instance();
    ui.autoFormat->setChecked(settings->autoFormatOnSave());
    const int index = ui.autoFormatTool->findText(settings->autoFormatTool());
    ui.autoFormatTool->setCurrentIndex(qMax(index, 0));
    ui.autoFormatMime->setText(settings->autoFormatMimeAsString());
    ui.autoFormatOnlyCurrentProject->setChecked(settings->autoFormatOnlyCurrentProject());
}

void GeneralOptionsPageWidget::apply()
{
    auto settings = GeneralSettings::instance();
    settings->setAutoFormatOnSave(ui.autoFormat->isChecked());
    settings->setAutoFormatTool(ui.autoFormatTool->currentText());
    settings->setAutoFormatMime(ui.autoFormatMime->text());
    settings->setAutoFormatOnlyCurrentProject(ui.autoFormatOnlyCurrentProject->isChecked());
    settings->save();
}

GeneralOptionsPage::GeneralOptionsPage(const QStringList &toolIds)
{
    setId(Constants::OPTION_GENERAL_ID);
    setDisplayName(GeneralOptionsPageWidget::tr("General"));
    setCategory(Constants::OPTION_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Beautifier", "Beautifier"));
    setWidgetCreator([toolIds] { return new GeneralOptionsPageWidget(toolIds); });
    setCategoryIconPath(":/beautifier/images/settingscategory_beautifier.png");
}

} // namespace Internal
} // namespace Beautifier
