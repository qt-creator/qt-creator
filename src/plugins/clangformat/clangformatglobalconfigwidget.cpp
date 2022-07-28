/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "clangformatglobalconfigwidget.h"

#include "clangformatconstants.h"
#include "clangformatsettings.h"

#include "ui_clangformatglobalconfigwidget.h"

#include <projectexplorer/project.h>

#include <sstream>

using namespace ProjectExplorer;

namespace ClangFormat {

ClangFormatGlobalConfigWidget::ClangFormatGlobalConfigWidget(ProjectExplorer::Project *project,
                                                             QWidget *parent)
    : CppCodeStyleWidget(parent)
    , m_ui(std::make_unique<Ui::ClangFormatGlobalConfigWidget>())
{
    m_ui->setupUi(this);

    initCheckBoxes();
    initIndentationOrFormattingCombobox();

    if (project) {
        m_ui->settingsGroupBox->hide();
        return;
    }
    m_ui->settingsGroupBox->show();
}

ClangFormatGlobalConfigWidget::~ClangFormatGlobalConfigWidget() = default;

void ClangFormatGlobalConfigWidget::initCheckBoxes()
{
    auto setEnableCheckBoxes = [this](int index) {
        bool isFormatting = index == static_cast<int>(ClangFormatSettings::Mode::Formatting);

        m_ui->formatOnSave->setEnabled(isFormatting);
        m_ui->formatWhileTyping->setEnabled(isFormatting);
    };
    setEnableCheckBoxes(m_ui->indentingOrFormatting->currentIndex());
    connect(m_ui->indentingOrFormatting, &QComboBox::currentIndexChanged,
            this, setEnableCheckBoxes);

    m_ui->formatOnSave->setChecked(ClangFormatSettings::instance().formatOnSave());
    m_ui->formatWhileTyping->setChecked(ClangFormatSettings::instance().formatWhileTyping());
}

void ClangFormatGlobalConfigWidget::initIndentationOrFormattingCombobox()
{
    m_ui->indentingOrFormatting->insertItem(static_cast<int>(ClangFormatSettings::Mode::Indenting),
                                            tr("Indenting only"));
    m_ui->indentingOrFormatting->insertItem(static_cast<int>(ClangFormatSettings::Mode::Formatting),
                                            tr("Full formatting"));
    m_ui->indentingOrFormatting->insertItem(static_cast<int>(ClangFormatSettings::Mode::Disable),
                                            tr("Disable"));

    m_ui->indentingOrFormatting->setCurrentIndex(
        static_cast<int>(ClangFormatSettings::instance().mode()));
}


void ClangFormatGlobalConfigWidget::apply()
{
    ClangFormatSettings &settings = ClangFormatSettings::instance();
    settings.setFormatOnSave(m_ui->formatOnSave->isChecked());
    settings.setFormatWhileTyping(m_ui->formatWhileTyping->isChecked());
    settings.setMode(static_cast<ClangFormatSettings::Mode>(m_ui->indentingOrFormatting->currentIndex()));
    settings.write();
}

} // namespace ClangFormat
