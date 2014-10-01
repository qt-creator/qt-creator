/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "bardescriptoreditorabstractpanelwidget.h"

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSignalMapper>
#include <QTextEdit>

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorEditorAbstractPanelWidget::BarDescriptorEditorAbstractPanelWidget(QWidget *parent) :
    QWidget(parent)
{
    m_signalMapper = new QSignalMapper(this);
    connect(m_signalMapper, SIGNAL(mapped(int)), this, SLOT(handleSignalMapped(int)));
}

void BarDescriptorEditorAbstractPanelWidget::setValue(BarDescriptorDocument::Tag tag, const QVariant &value)
{
    if (m_blockedSignals.contains(tag))
        return;

    blockSignalMapping(tag);
    updateWidgetValue(tag, value);
    unblockSignalMapping(tag);
}

void BarDescriptorEditorAbstractPanelWidget::addSignalMapping(BarDescriptorDocument::Tag tag, QObject *object, const char *signal)
{
    m_signalMapper->setMapping(object, tag);
    connect(object, signal, m_signalMapper, SLOT(map()));
}

void BarDescriptorEditorAbstractPanelWidget::blockSignalMapping(BarDescriptorDocument::Tag tag)
{
    m_blockedSignals.prepend(tag);
}

void BarDescriptorEditorAbstractPanelWidget::unblockSignalMapping(BarDescriptorDocument::Tag tag)
{
    BarDescriptorDocument::Tag removedTag = m_blockedSignals.takeFirst();
    QTC_CHECK(removedTag == tag);
}

void BarDescriptorEditorAbstractPanelWidget::updateWidgetValue(BarDescriptorDocument::Tag tag, const QVariant &value)
{
    QObject *object = m_signalMapper->mapping(static_cast<int>(tag));
    if (!object)
        return;

    if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(object))
        lineEdit->setText(value.toString());
    else if (QTextEdit *textEdit = qobject_cast<QTextEdit *>(object))
        textEdit->setPlainText(value.toString());
    else if (Utils::PathChooser *pathChooser = qobject_cast<Utils::PathChooser *>(object))
        pathChooser->setPath(value.toString());
    else if (QComboBox *comboBox = qobject_cast<QComboBox *>(object))
        comboBox->setCurrentIndex(comboBox->findData(value.toString()));
    else if (QCheckBox *checkBox = qobject_cast<QCheckBox *>(object))
        checkBox->setChecked(value.toBool());
    else
        QTC_CHECK(false);
}

void BarDescriptorEditorAbstractPanelWidget::emitChanged(BarDescriptorDocument::Tag tag)
{
    QObject *sender = m_signalMapper->mapping(tag);

    if (!sender)
        return;

    if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(sender))
        emit changed(tag, lineEdit->text());
    else if (QTextEdit *textEdit = qobject_cast<QTextEdit *>(sender))
        emit changed(tag, textEdit->toPlainText());
    else if (Utils::PathChooser *pathChooser = qobject_cast<Utils::PathChooser *>(sender))
        emit changed(tag, pathChooser->path());
    else if (QComboBox *comboBox = qobject_cast<QComboBox *>(sender))
        emit changed(tag, comboBox->itemData(comboBox->currentIndex()));
    else if (QCheckBox *checkBox = qobject_cast<QCheckBox *>(sender))
        emit changed(tag, checkBox->isChecked());
    else
        QTC_CHECK(false);
}

void BarDescriptorEditorAbstractPanelWidget::handleSignalMapped(int id)
{
    BarDescriptorDocument::Tag tag = static_cast<BarDescriptorDocument::Tag>(id);

    if (m_blockedSignals.contains(tag))
        return;

    blockSignalMapping(tag);
    emitChanged(tag);
    unblockSignalMapping(tag);
}
