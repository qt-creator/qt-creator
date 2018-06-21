/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "tabsettingswidget.h"
#include "ui_tabsettingswidget.h"
#include "tabsettings.h"

#include <QTextStream>

namespace TextEditor {

TabSettingsWidget::TabSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Internal::Ui::TabSettingsWidget)
{
    ui->setupUi(this);
    ui->codingStyleWarning->setVisible(false);

    auto comboIndexChanged = static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged);
    auto spinValueChanged = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged);
    connect(ui->codingStyleWarning, &QLabel::linkActivated,
            this, &TabSettingsWidget::codingStyleLinkActivated);
    connect(ui->tabPolicy, comboIndexChanged,
            this, &TabSettingsWidget::slotSettingsChanged);
    connect(ui->tabSize, spinValueChanged,
            this, &TabSettingsWidget::slotSettingsChanged);
    connect(ui->indentSize, spinValueChanged,
            this, &TabSettingsWidget::slotSettingsChanged);
    connect(ui->continuationAlignBehavior, comboIndexChanged,
            this, &TabSettingsWidget::slotSettingsChanged);
}

TabSettingsWidget::~TabSettingsWidget()
{
    delete ui;
}

void TabSettingsWidget::setTabSettings(const TextEditor::TabSettings& s)
{
    QSignalBlocker blocker(this);
    ui->tabPolicy->setCurrentIndex(s.m_tabPolicy);
    ui->tabSize->setValue(s.m_tabSize);
    ui->indentSize->setValue(s.m_indentSize);
    ui->continuationAlignBehavior->setCurrentIndex(s.m_continuationAlignBehavior);
}

TabSettings TabSettingsWidget::tabSettings() const
{
    TabSettings set;

    set.m_tabPolicy = (TabSettings::TabPolicy)(ui->tabPolicy->currentIndex());
    set.m_tabSize = ui->tabSize->value();
    set.m_indentSize = ui->indentSize->value();
    set.m_continuationAlignBehavior = (TabSettings::ContinuationAlignBehavior)(ui->continuationAlignBehavior->currentIndex());

    return set;
}

void TabSettingsWidget::slotSettingsChanged()
{
    emit settingsChanged(tabSettings());
}

void TabSettingsWidget::codingStyleLinkActivated(const QString &linkString)
{
    if (linkString == QLatin1String("C++"))
        emit codingStyleLinkClicked(CppLink);
    else if (linkString == QLatin1String("QtQuick"))
        emit codingStyleLinkClicked(QtQuickLink);
}

void TabSettingsWidget::setFlat(bool on)
{
    ui->tabsAndIndentationGroupBox->setFlat(on);
    const int margin = on ? 0 : -1;
    ui->tabsAndIndentationGroupBox->layout()->setContentsMargins(margin, -1, margin, margin);
}

void TabSettingsWidget::setCodingStyleWarningVisible(bool visible)
{
    ui->codingStyleWarning->setVisible(visible);
}

} // namespace TextEditor
