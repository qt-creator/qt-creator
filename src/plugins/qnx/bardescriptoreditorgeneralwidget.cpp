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

    connect(m_ui->orientation, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));
    connect(m_ui->chrome, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));
    connect(m_ui->transparentMainWindow, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_ui->applicationArguments, SIGNAL(textChanged(QString)), this, SIGNAL(changed()));
}

BarDescriptorEditorGeneralWidget::~BarDescriptorEditorGeneralWidget()
{
    delete m_ui;
}

void BarDescriptorEditorGeneralWidget::clear()
{
    setComboBoxBlocked(m_ui->orientation, m_ui->orientation->findData(QLatin1String("")));
    setComboBoxBlocked(m_ui->chrome, m_ui->chrome->findData(QLatin1String("none")));
    setCheckBoxBlocked(m_ui->transparentMainWindow, false);
    setLineEditBlocked(m_ui->applicationArguments, QString());
}

QString BarDescriptorEditorGeneralWidget::orientation() const
{
    return m_ui->orientation->itemData(m_ui->orientation->currentIndex()).toString();
}

void BarDescriptorEditorGeneralWidget::setOrientation(const QString &orientation)
{
    int index = m_ui->orientation->findData(orientation);
    QTC_ASSERT(index >= 0, return);

    setComboBoxBlocked(m_ui->orientation, index);
}

QString BarDescriptorEditorGeneralWidget::chrome() const
{
    return m_ui->chrome->itemData(m_ui->chrome->currentIndex()).toString();
}

void BarDescriptorEditorGeneralWidget::setChrome(const QString &chrome)
{
    int index = m_ui->chrome->findData(chrome);
    QTC_ASSERT(index >= 0, return);

    setComboBoxBlocked(m_ui->chrome, index);
}

bool BarDescriptorEditorGeneralWidget::transparent() const
{
    return m_ui->transparentMainWindow->isChecked();
}

void BarDescriptorEditorGeneralWidget::setTransparent(bool transparent)
{
    setCheckBoxBlocked(m_ui->transparentMainWindow, transparent);
}

void BarDescriptorEditorGeneralWidget::appendApplicationArgument(const QString &argument)
{
    QString completeArguments = m_ui->applicationArguments->text();
    if (!completeArguments.isEmpty())
        completeArguments.append(QLatin1Char(' '));
    completeArguments.append(argument);

    setLineEditBlocked(m_ui->applicationArguments, completeArguments);
}

QStringList BarDescriptorEditorGeneralWidget::applicationArguments() const
{
    // TODO: Should probably handle "argument with spaces within quotes"
    return m_ui->applicationArguments->text().split(QLatin1Char(' '));
}
