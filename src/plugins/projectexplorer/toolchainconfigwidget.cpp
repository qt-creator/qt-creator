// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolchainconfigwidget.h"

#include "toolchain.h"
#include "projectexplorertr.h"

#include <utils/detailswidget.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QScrollArea>
#include <QPainter>

using namespace Utils;

namespace ProjectExplorer {

ToolChainConfigWidget::ToolChainConfigWidget(ToolChain *tc) :
    m_toolChain(tc)
{
    Q_ASSERT(tc);

    auto centralWidget = new Utils::DetailsWidget;
    centralWidget->setState(Utils::DetailsWidget::NoSummary);

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

    m_mainLayout->addRow(Tr::tr("Name:"), m_nameLineEdit);

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

QStringList ToolChainConfigWidget::splitString(const QString &s)
{
    ProcessArgs::SplitError splitError;
    const OsType osType = HostOsInfo::hostOs();
    QStringList res = ProcessArgs::splitArgs(s, osType, false, &splitError);
    if (splitError != ProcessArgs::SplitOk) {
        res = ProcessArgs::splitArgs(s + '\\', osType, false, &splitError);
        if (splitError != ProcessArgs::SplitOk) {
            res = ProcessArgs::splitArgs(s + '"', osType, false, &splitError);
            if (splitError != ProcessArgs::SplitOk)
                res = ProcessArgs::splitArgs(s + '\'', osType, false, &splitError);
        }
    }
    return res;
}

} // namespace ProjectExplorer
