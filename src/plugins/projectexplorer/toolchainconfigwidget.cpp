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

#include "toolchainconfigwidget.h"
#include "toolchain.h"

#include <utils/qtcassert.h>

#include <QString>

#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>

namespace ProjectExplorer {

ToolChainConfigWidget::ToolChainConfigWidget(ToolChain *tc) :
    m_toolChain(tc), m_errorLabel(0)
{
    QTC_CHECK(tc);
    m_nameLineEdit = new QLineEdit(this);
    m_nameLineEdit->setText(tc->displayName());
    m_mainLayout = new QFormLayout(this);
    m_mainLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow); // for the Macs...
    m_mainLayout->addRow(tr("Name:"), m_nameLineEdit);

    connect(m_nameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(dirty()));
}

void ToolChainConfigWidget::apply()
{
    m_toolChain->setDisplayName(m_nameLineEdit->text());
    applyImpl();
}

void ToolChainConfigWidget::discard()
{
    m_nameLineEdit->setText(m_toolChain->displayName());
    discardImpl();
}

bool ToolChainConfigWidget::isDirty() const
{
    return m_nameLineEdit->text() != m_toolChain->displayName() || isDirtyImpl();
}

ToolChain *ToolChainConfigWidget::toolChain() const
{
    return m_toolChain;
}

void ToolChainConfigWidget::makeReadOnly()
{
    m_nameLineEdit->setEnabled(false);
    makeReadOnlyImpl();
}

void ToolChainConfigWidget::addErrorLabel()
{
    if (!m_errorLabel) {
        m_errorLabel = new QLabel;
        m_errorLabel->setVisible(false);
    }
    m_mainLayout->addRow(m_errorLabel);
}

void ToolChainConfigWidget::setErrorMessage(const QString &m)
{
    QTC_ASSERT(m_errorLabel, return);
    if (m.isEmpty()) {
        clearErrorMessage();
    } else {
        m_errorLabel->setText(m);
        m_errorLabel->setStyleSheet(QLatin1String("background-color: \"red\""));
        m_errorLabel->setVisible(true);
    }
}

void ToolChainConfigWidget::clearErrorMessage()
{
    QTC_ASSERT(m_errorLabel, return);
    m_errorLabel->clear();
    m_errorLabel->setStyleSheet(QString());
    m_errorLabel->setVisible(false);
}

} // namespace ProjectExplorer
