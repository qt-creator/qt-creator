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

#include "bardescriptoreditorgeneralwidget.h"
#include "ui_bardescriptoreditorgeneralwidget.h"

#include <utils/qtcassert.h>

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorEditorGeneralWidget::BarDescriptorEditorGeneralWidget(QWidget *parent) :
    BarDescriptorEditorAbstractPanelWidget(parent),
    m_ui(new Ui::BarDescriptorEditorGeneralWidget)
{
    m_ui->setupUi(this);

    m_ui->orientation->addItem(tr("Default"), QLatin1String(""));
    m_ui->orientation->addItem(tr("Auto-orient"), QLatin1String("auto-orient"));
    m_ui->orientation->addItem(tr("Landscape"), QLatin1String("landscape"));
    m_ui->orientation->addItem(tr("Portrait"), QLatin1String("portrait"));

    m_ui->chrome->addItem(tr("Standard"), QLatin1String("standard"));
    m_ui->chrome->addItem(tr("None"), QLatin1String("none"));

    addSignalMapping(BarDescriptorDocument::aspectRatio, m_ui->orientation, SIGNAL(currentIndexChanged(int)));
    addSignalMapping(BarDescriptorDocument::systemChrome, m_ui->chrome, SIGNAL(currentIndexChanged(int)));
    addSignalMapping(BarDescriptorDocument::transparent, m_ui->transparentMainWindow, SIGNAL(toggled(bool)));
    addSignalMapping(BarDescriptorDocument::arg, m_ui->applicationArguments, SIGNAL(textChanged(QString)));
}

BarDescriptorEditorGeneralWidget::~BarDescriptorEditorGeneralWidget()
{
    delete m_ui;
}

void BarDescriptorEditorGeneralWidget::updateWidgetValue(BarDescriptorDocument::Tag tag, const QVariant &value)
{
    if (tag == BarDescriptorDocument::aspectRatio) {
        m_ui->orientation->setCurrentIndex(m_ui->orientation->findData(value));
    } else if (tag == BarDescriptorDocument::autoOrients) {
        if (value.toString() == QLatin1String("true")) {
            blockSignalMapping(BarDescriptorDocument::aspectRatio);
            m_ui->orientation->setCurrentIndex(m_ui->orientation->findData(QLatin1String("auto-orient")));
            unblockSignalMapping(BarDescriptorDocument::aspectRatio);
        }
    } else if (tag == BarDescriptorDocument::arg) {
        m_ui->applicationArguments->setText(value.toStringList().join(QLatin1Char(' ')));
    } else {
        BarDescriptorEditorAbstractPanelWidget::updateWidgetValue(tag, value);
    }
}

void BarDescriptorEditorGeneralWidget::emitChanged(BarDescriptorDocument::Tag tag)
{
    if (tag == BarDescriptorDocument::aspectRatio) {
        QString value = m_ui->orientation->itemData(m_ui->orientation->currentIndex()).toString();
        if (value == QLatin1String("auto-orient")) {
            emit changed(BarDescriptorDocument::aspectRatio, QLatin1String(""));
            emit changed(BarDescriptorDocument::autoOrients, QLatin1String("true"));
            return;
        } else if (!value.isEmpty()) {
            emit changed(BarDescriptorDocument::aspectRatio, value);
            emit changed(BarDescriptorDocument::autoOrients, QLatin1String("false"));
        } else {
            emit changed(BarDescriptorDocument::aspectRatio, value);
            emit changed(BarDescriptorDocument::autoOrients, QLatin1String(""));
        }
    } else if (tag == BarDescriptorDocument::arg) {
        emit changed(tag, m_ui->applicationArguments->text().split(QLatin1Char(' ')));
    } else {
        BarDescriptorEditorAbstractPanelWidget::emitChanged(tag);
    }
}
