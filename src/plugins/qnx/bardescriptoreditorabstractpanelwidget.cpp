/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "bardescriptoreditorabstractpanelwidget.h"

#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorEditorAbstractPanelWidget::BarDescriptorEditorAbstractPanelWidget(QWidget *parent) :
    QWidget(parent)
{
}


void BarDescriptorEditorAbstractPanelWidget::setComboBoxBlocked(QComboBox *comboBox, int index)
{
    bool blocked = comboBox->blockSignals(true);
    comboBox->setCurrentIndex(index);
    comboBox->blockSignals(blocked);
}

void BarDescriptorEditorAbstractPanelWidget::setCheckBoxBlocked(QCheckBox *checkBox, bool checked)
{
    bool blocked = checkBox->blockSignals(true);
    checkBox->setChecked(checked);
    checkBox->blockSignals(blocked);
}

void BarDescriptorEditorAbstractPanelWidget::setLineEditBlocked(QLineEdit *lineEdit, const QString &text)
{
    bool blocked = lineEdit->blockSignals(true);
    lineEdit->setText(text);
    lineEdit->blockSignals(blocked);
}

void BarDescriptorEditorAbstractPanelWidget::setTextEditBlocked(QTextEdit *textEdit, const QString &text)
{
    bool blocked = textEdit->blockSignals(true);
    textEdit->setPlainText(text);
    textEdit->blockSignals(blocked);
}

void BarDescriptorEditorAbstractPanelWidget::setPathChooserBlocked(Utils::PathChooser *pathChooser, const QString &path)
{
    bool blocked = pathChooser->blockSignals(true);
    pathChooser->setPath(path);
    pathChooser->blockSignals(blocked);
}
