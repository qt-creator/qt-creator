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
    RunConfigWidget(),
    m_aspect(aspect),
    m_ignoreChange(false),
    m_additionalWidget(additionalWidget)
{
    QTC_CHECK(m_aspect);

    setContentsMargins(0, 0, 0, 0);
    auto topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);

    auto baseEnvironmentWidget = new QWidget;
    auto baseLayout = new QHBoxLayout(baseEnvironmentWidget);
    baseLayout->setMargin(0);
    auto label = new QLabel(tr("Base environment for this run configuration:"), this);
    baseLayout->addWidget(label);
    m_baseEnvironmentComboBox = new QComboBox;
    QList<int> bases = m_aspect->possibleBaseEnvironments();
    int currentBase = m_aspect->baseEnvironmentBase();
    QString baseDisplayName;
    foreach (int i, bases) {
        const QString displayName = m_aspect->baseEnvironmentDisplayName(i);
        m_baseEnvironmentComboBox->addItem(displayName, i);
        if (i == currentBase) {
            m_baseEnvironmentComboBox->setCurrentIndex(m_baseEnvironmentComboBox->count() - 1);
            baseDisplayName = displayName;
        }
    }
    if (m_baseEnvironmentComboBox->count() == 1)
        m_baseEnvironmentComboBox->setEnabled(false);

    connect(m_baseEnvironmentComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &EnvironmentAspectWidget::baseEnvironmentSelected);

    baseLayout->addWidget(m_baseEnvironmentComboBox);
    baseLayout->addStretch(10);
    if (additionalWidget)
        baseLayout->addWidget(additionalWidget);

    m_environmentWidget = new EnvironmentWidget(this, baseEnvironmentWidget);
    m_environmentWidget->setBaseEnvironment(m_aspect->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(baseDisplayName);
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

QString EnvironmentAspectWidget::displayName() const
{
    return m_aspect->displayName();
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
    int base = m_baseEnvironmentComboBox->itemData(idx).toInt();
    m_aspect->setBaseEnvironmentBase(base);
    m_environmentWidget->setBaseEnvironment(m_aspect->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_aspect->baseEnvironmentDisplayName(base));
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
    m_environmentWidget->setBaseEnvironmentText(m_aspect->baseEnvironmentDisplayName(base));
    m_environmentWidget->setBaseEnvironment(m_aspect->baseEnvironment());
}

void EnvironmentAspectWidget::userChangesEdited()
{
    m_ignoreChange = true;
    m_aspect->setUserEnvironmentChanges(m_environmentWidget->userChanges());
    m_ignoreChange = false;
}

void EnvironmentAspectWidget::changeUserChanges(QList<Utils::EnvironmentItem> changes)
{
    if (m_ignoreChange)
        return;
    m_environmentWidget->setUserChanges(changes);
}

void EnvironmentAspectWidget::environmentChanged()
{
    if (m_ignoreChange)
        return;
    m_environmentWidget->setBaseEnvironment(m_aspect->baseEnvironment());
}

} // namespace ProjectExplorer
