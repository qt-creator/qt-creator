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

#include "environmentaspectwidget.h"

#include "environmentwidget.h"

#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace ProjectExplorer {

// --------------------------------------------------------------------
// EnvironmentAspectWidget:
// --------------------------------------------------------------------

EnvironmentAspectWidget::EnvironmentAspectWidget(EnvironmentAspect *aspect, QWidget *additionalWidget) :
    m_aspect(aspect),
    m_additionalWidget(additionalWidget)
{
    QTC_CHECK(m_aspect);

    setContentsMargins(0, 0, 0, 0);
    auto topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 25);

    auto baseEnvironmentWidget = new QWidget;
    auto baseLayout = new QHBoxLayout(baseEnvironmentWidget);
    baseLayout->setContentsMargins(0, 0, 0, 0);
    auto label = new QLabel(tr("Base environment for this run configuration:"), this);
    baseLayout->addWidget(label);

    m_baseEnvironmentComboBox = new QComboBox;
    for (const QString &displayName : m_aspect->displayNames())
        m_baseEnvironmentComboBox->addItem(displayName);
    if (m_baseEnvironmentComboBox->count() == 1)
        m_baseEnvironmentComboBox->setEnabled(false);
    m_baseEnvironmentComboBox->setCurrentIndex(m_aspect->baseEnvironmentBase());

    connect(m_baseEnvironmentComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EnvironmentAspectWidget::baseEnvironmentSelected);

    baseLayout->addWidget(m_baseEnvironmentComboBox);
    baseLayout->addStretch(10);
    if (additionalWidget)
        baseLayout->addWidget(additionalWidget);

    const EnvironmentWidget::Type widgetType = aspect->isLocal()
            ? EnvironmentWidget::TypeLocal : EnvironmentWidget::TypeRemote;
    m_environmentWidget = new EnvironmentWidget(this, widgetType, baseEnvironmentWidget);
    m_environmentWidget->setBaseEnvironment(m_aspect->modifiedBaseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_aspect->currentDisplayName());
    m_environmentWidget->setUserChanges(m_aspect->userEnvironmentChanges());
    m_environmentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    topLayout->addWidget(m_environmentWidget);

    connect(m_environmentWidget, &EnvironmentWidget::userChangesChanged,
            this, &EnvironmentAspectWidget::userChangesEdited);

    connect(m_aspect, &EnvironmentAspect::baseEnvironmentChanged,
            this, &EnvironmentAspectWidget::changeBaseEnvironment);
    connect(m_aspect, &EnvironmentAspect::userEnvironmentChangesChanged,
            this, &EnvironmentAspectWidget::changeUserChanges);
    connect(m_aspect, &EnvironmentAspect::environmentChanged,
            this, &EnvironmentAspectWidget::environmentChanged);
}

EnvironmentAspect *EnvironmentAspectWidget::aspect() const
{
    return m_aspect;
}

QWidget *EnvironmentAspectWidget::additionalWidget() const
{
    return m_additionalWidget;
}

void EnvironmentAspectWidget::baseEnvironmentSelected(int idx)
{
    m_ignoreChange = true;
    m_aspect->setBaseEnvironmentBase(idx);
    m_environmentWidget->setBaseEnvironment(m_aspect->modifiedBaseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_aspect->currentDisplayName());
    m_ignoreChange = false;
}

void EnvironmentAspectWidget::changeBaseEnvironment()
{
    if (m_ignoreChange)
        return;

    int base = m_aspect->baseEnvironmentBase();
    for (int i = 0; i < m_baseEnvironmentComboBox->count(); ++i) {
        if (m_baseEnvironmentComboBox->itemData(i).toInt() == base)
            m_baseEnvironmentComboBox->setCurrentIndex(i);
    }
    m_environmentWidget->setBaseEnvironmentText(m_aspect->currentDisplayName());
    m_environmentWidget->setBaseEnvironment(m_aspect->modifiedBaseEnvironment());
}

void EnvironmentAspectWidget::userChangesEdited()
{
    m_ignoreChange = true;
    m_aspect->setUserEnvironmentChanges(m_environmentWidget->userChanges());
    m_ignoreChange = false;
}

void EnvironmentAspectWidget::changeUserChanges(Utils::EnvironmentItems changes)
{
    if (m_ignoreChange)
        return;
    m_environmentWidget->setUserChanges(changes);
}

void EnvironmentAspectWidget::environmentChanged()
{
    if (m_ignoreChange)
        return;
    m_environmentWidget->setBaseEnvironment(m_aspect->modifiedBaseEnvironment());
}

} // namespace ProjectExplorer
