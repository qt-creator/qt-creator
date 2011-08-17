/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "tabsettingswidget.h"
#include "ui_tabsettingswidget.h"
#include "tabsettings.h"

#include <QtCore/QTextStream>

namespace TextEditor {

TabSettingsWidget::TabSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TabSettingsWidget)
{
    ui->setupUi(this);

    connect(ui->insertSpaces, SIGNAL(toggled(bool)),
            this, SLOT(slotSettingsChanged()));
    connect(ui->insertSpaces, SIGNAL(toggled(bool)),
            this, SLOT(updateWidget()));
    connect(ui->autoInsertSpaces, SIGNAL(toggled(bool)),
            this, SLOT(slotSettingsChanged()));
    connect(ui->autoIndent, SIGNAL(toggled(bool)),
            this, SLOT(slotSettingsChanged()));
    connect(ui->smartBackspaceBehavior, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotSettingsChanged()));
    connect(ui->tabSize, SIGNAL(valueChanged(int)),
            this, SLOT(slotSettingsChanged()));
    connect(ui->indentSize, SIGNAL(valueChanged(int)),
            this, SLOT(slotSettingsChanged()));
    connect(ui->tabKeyBehavior, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotSettingsChanged()));
    connect(ui->continuationAlignBehavior, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotSettingsChanged()));

    setFlat(false);
}

TabSettingsWidget::~TabSettingsWidget()
{
    delete ui;
}

void TabSettingsWidget::setSettings(const TextEditor::TabSettings& s)
{
    const bool wasBlocked = blockSignals(true);
    ui->insertSpaces->setChecked(s.m_spacesForTabs);
    ui->autoInsertSpaces->setChecked(s.m_autoSpacesForTabs);
    ui->autoIndent->setChecked(s.m_autoIndent);
    ui->smartBackspaceBehavior->setCurrentIndex(s.m_smartBackspaceBehavior);
    ui->tabSize->setValue(s.m_tabSize);
    ui->indentSize->setValue(s.m_indentSize);
    ui->tabKeyBehavior->setCurrentIndex(s.m_tabKeyBehavior);
    ui->continuationAlignBehavior->setCurrentIndex(s.m_continuationAlignBehavior);
    blockSignals(wasBlocked);

    updateWidget();
}

TabSettings TabSettingsWidget::settings() const
{
    TabSettings set;

    set.m_spacesForTabs = ui->insertSpaces->isChecked();
    set.m_autoSpacesForTabs = ui->autoInsertSpaces->isChecked();
    set.m_autoIndent = ui->autoIndent->isChecked();
    set.m_smartBackspaceBehavior =
        (TabSettings::SmartBackspaceBehavior)ui->smartBackspaceBehavior->currentIndex();
    set.m_tabSize = ui->tabSize->value();
    set.m_indentSize = ui->indentSize->value();
    set.m_tabKeyBehavior = (TabSettings::TabKeyBehavior)(ui->tabKeyBehavior->currentIndex());
    set.m_continuationAlignBehavior =
        (TabSettings::ContinuationAlignBehavior)(ui->continuationAlignBehavior->currentIndex());

    return set;
}

void TabSettingsWidget::slotSettingsChanged()
{
    emit settingsChanged(settings());
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
            << sep << ui->insertSpaces->text()
            << sep << ui->autoInsertSpaces->text()
            << sep << ui->autoIndent->text()
            << sep << ui->smartBackspaceLabel->text()
            << sep << ui->tabSizeLabel->text()
            << sep << ui->indentSizeLabel->text()
            << sep << ui->tabKeyBehaviorLabel->text()
            << sep << ui->continuationAlignBehaviorLabel->text()
               ;
    rc.remove(QLatin1Char('&'));
    return rc;
}

void TabSettingsWidget::updateWidget()
{
    ui->autoInsertSpaces->setEnabled(ui->insertSpaces->isChecked());
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
