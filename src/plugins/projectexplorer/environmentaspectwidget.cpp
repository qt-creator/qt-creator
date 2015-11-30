/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);

    QWidget *baseEnvironmentWidget = new QWidget;
    QHBoxLayout *baseLayout = new QHBoxLayout(baseEnvironmentWidget);
    baseLayout->setMargin(0);
    QLabel *label = new QLabel(tr("Base environment for this run configuration:"), this);
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

    connect(m_baseEnvironmentComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(baseEnvironmentSelected(int)));

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

    connect(m_environmentWidget, SIGNAL(userChangesChanged()),
            this, SLOT(userChangesEdited()));

    connect(m_aspect, SIGNAL(baseEnvironmentChanged()), this, SLOT(changeBaseEnvironment()));
    connect(m_aspect, SIGNAL(userEnvironmentChangesChanged(QList<Utils::EnvironmentItem>)),
            this, SLOT(changeUserChanges(QList<Utils::EnvironmentItem>)));
    connect(m_aspect, SIGNAL(environmentChanged()),
            this, SLOT(environmentChanged()));
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
