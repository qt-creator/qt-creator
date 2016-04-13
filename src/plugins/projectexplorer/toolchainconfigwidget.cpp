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

#include "toolchainconfigwidget.h"
#include "toolchain.h"

#include <utils/detailswidget.h>
#include <utils/qtcassert.h>

#include <QString>

#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QScrollArea>
#include <QPainter>

namespace ProjectExplorer {

ToolChainConfigWidget::ToolChainConfigWidget(ToolChain *tc) :
    m_toolChain(tc)
{
    Q_ASSERT(tc);

    auto centralWidget = new Utils::DetailsWidget;
    centralWidget->setState(Utils::DetailsWidget::NoSummary);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    setFocusPolicy(Qt::NoFocus);

    setWidget(centralWidget);

    auto detailsBox = new QWidget();

    m_mainLayout = new QFormLayout(detailsBox);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    centralWidget->setWidget(detailsBox);
    m_mainLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow); // for the Macs...

    m_nameLineEdit = new QLineEdit;
    m_nameLineEdit->setText(tc->displayName());

    m_mainLayout->addRow(tr("Name:"), m_nameLineEdit);

    connect(m_nameLineEdit, &QLineEdit::textChanged, this, &ToolChainConfigWidget::dirty);
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
