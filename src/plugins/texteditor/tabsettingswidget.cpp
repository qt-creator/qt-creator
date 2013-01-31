/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

    connect(ui->tabPolicy, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotSettingsChanged()));
    connect(ui->tabSize, SIGNAL(valueChanged(int)),
            this, SLOT(slotSettingsChanged()));
    connect(ui->indentSize, SIGNAL(valueChanged(int)),
            this, SLOT(slotSettingsChanged()));
    connect(ui->continuationAlignBehavior, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotSettingsChanged()));
}

TabSettingsWidget::~TabSettingsWidget()
{
    delete ui;
}

void TabSettingsWidget::setTabSettings(const TextEditor::TabSettings& s)
{
    const bool wasBlocked = blockSignals(true);
    ui->tabPolicy->setCurrentIndex(s.m_tabPolicy);
    ui->tabSize->setValue(s.m_tabSize);
    ui->indentSize->setValue(s.m_indentSize);
    ui->continuationAlignBehavior->setCurrentIndex(s.m_continuationAlignBehavior);
    blockSignals(wasBlocked);
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

void TabSettingsWidget::setFlat(bool on)
{
    ui->tabsAndIndentationGroupBox->setFlat(on);
    const int margin = on ? 0 : -1;
    ui->tabsAndIndentationGroupBox->layout()->setContentsMargins(margin, -1, margin, margin);
}

QString TabSettingsWidget::searchKeywords() const
{
    QString rc;
    QLatin1Char sep(' ');
    QTextStream(&rc)
            << sep << ui->tabsAndIndentationGroupBox->title()
            << sep << ui->tabPolicyLabel->text()
            << sep << ui->tabSizeLabel->text()
            << sep << ui->indentSizeLabel->text()
            << sep << ui->continuationAlignBehaviorLabel->text()
               ;
    rc.remove(QLatin1Char('&'));
    return rc;
}

void TabSettingsWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

} // namespace TextEditor
