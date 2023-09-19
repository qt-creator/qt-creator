// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "environmentaspectwidget.h"

#include "environmentwidget.h"
#include "projectexplorertr.h"

#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace ProjectExplorer {

// --------------------------------------------------------------------
// EnvironmentAspectWidget:
// --------------------------------------------------------------------

EnvironmentAspectWidget::EnvironmentAspectWidget(EnvironmentAspect *aspect)
    : m_aspect(aspect)
{
    QTC_CHECK(m_aspect);

    setContentsMargins(0, 0, 0, 0);
    auto topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 25);

    auto baseEnvironmentWidget = new QWidget;
    m_baseLayout = new QHBoxLayout(baseEnvironmentWidget);
    m_baseLayout->setContentsMargins(0, 0, 0, 0);

    auto label = [aspect]() {
        if (aspect->labelText().isEmpty())
            aspect->setLabelText(Tr::tr("Base environment for this run configuration:"));
        return aspect->createLabel();
    };
    m_baseLayout->addWidget(label());

    m_baseEnvironmentComboBox = new QComboBox;
    for (const QString &displayName : m_aspect->displayNames())
        m_baseEnvironmentComboBox->addItem(displayName);
    if (m_baseEnvironmentComboBox->count() == 1)
        m_baseEnvironmentComboBox->setEnabled(false);
    m_baseEnvironmentComboBox->setCurrentIndex(m_aspect->baseEnvironmentBase());

    connect(m_baseEnvironmentComboBox, &QComboBox::currentIndexChanged,
            this, &EnvironmentAspectWidget::baseEnvironmentSelected);

    m_baseLayout->addWidget(m_baseEnvironmentComboBox);
    m_baseLayout->addStretch(10);

    const EnvironmentWidget::Type widgetType = aspect->isLocal()
            ? EnvironmentWidget::TypeLocal : EnvironmentWidget::TypeRemote;
    m_environmentWidget = new EnvironmentWidget(this, widgetType, baseEnvironmentWidget);
    m_environmentWidget->setBaseEnvironment(m_aspect->modifiedBaseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_aspect->currentDisplayName());
    m_environmentWidget->setUserChanges(m_aspect->userEnvironmentChanges());
    m_environmentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    topLayout->addWidget(m_environmentWidget);

    if (m_aspect->isPrintOnRunAllowed()) {
        const auto printOnRunCheckBox = new QCheckBox(
            Tr::tr("Show in Application Output when running"));
        printOnRunCheckBox->setChecked(m_aspect->isPrintOnRunEnabled());
        connect(printOnRunCheckBox, &QCheckBox::toggled,
                m_aspect, &EnvironmentAspect::setPrintOnRun);
        topLayout->addWidget(printOnRunCheckBox);
    }

    connect(m_environmentWidget, &EnvironmentWidget::userChangesChanged,
            this, &EnvironmentAspectWidget::userChangesEdited);

    connect(m_aspect, &EnvironmentAspect::baseEnvironmentChanged,
            this, &EnvironmentAspectWidget::changeBaseEnvironment);
    connect(m_aspect, &EnvironmentAspect::userEnvironmentChangesChanged,
            this, &EnvironmentAspectWidget::changeUserChanges);
    connect(m_aspect, &EnvironmentAspect::environmentChanged,
            this, &EnvironmentAspectWidget::environmentChanged);
}

void EnvironmentAspectWidget::addWidget(QWidget *widget)
{
    m_baseLayout->addWidget(widget);
}

void EnvironmentAspectWidget::baseEnvironmentSelected(int idx)
{
    const Utils::GuardLocker locker(m_ignoreChanges);
    m_aspect->setBaseEnvironmentBase(idx);
    m_environmentWidget->setBaseEnvironment(m_aspect->modifiedBaseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_aspect->currentDisplayName());
}

void EnvironmentAspectWidget::changeBaseEnvironment()
{
    if (m_ignoreChanges.isLocked())
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
    const Utils::GuardLocker locker(m_ignoreChanges);
    m_aspect->setUserEnvironmentChanges(m_environmentWidget->userChanges());
}

void EnvironmentAspectWidget::changeUserChanges(Utils::EnvironmentItems changes)
{
    if (m_ignoreChanges.isLocked())
        return;
    m_environmentWidget->setUserChanges(changes);
}

void EnvironmentAspectWidget::environmentChanged()
{
    if (m_ignoreChanges.isLocked())
        return;
    m_environmentWidget->setBaseEnvironment(m_aspect->modifiedBaseEnvironment());
}

} // namespace ProjectExplorer
