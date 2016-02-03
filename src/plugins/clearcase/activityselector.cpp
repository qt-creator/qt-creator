/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include "activityselector.h"
#include "clearcaseconstants.h"
#include "clearcaseplugin.h"

#include <utils/qtcassert.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

using namespace ClearCase;
using namespace ClearCase::Internal;

ActivitySelector::ActivitySelector(QWidget *parent) : QWidget(parent),
    m_plugin(ClearCasePlugin::instance())
{
    QTC_ASSERT(m_plugin->isUcm(), return);

    auto hboxLayout = new QHBoxLayout(this);
    hboxLayout->setContentsMargins(0, 0, 0, 0);

    auto lblActivity = new QLabel(tr("Select &activity:"));
    lblActivity->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    hboxLayout->addWidget(lblActivity);

    m_cmbActivity = new QComboBox(this);
    m_cmbActivity->setMinimumSize(QSize(350, 0));
    hboxLayout->addWidget(m_cmbActivity);

    QString addText = tr("Add");
    if (!m_plugin->settings().autoAssignActivityName)
        addText.append(QLatin1String("..."));
    auto btnAdd = new QToolButton;
    btnAdd->setText(addText);
    hboxLayout->addWidget(btnAdd);

    lblActivity->setBuddy(m_cmbActivity);

    connect(btnAdd, &QToolButton::clicked, this, &ActivitySelector::newActivity);

    refresh();
    connect(m_cmbActivity, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ActivitySelector::userChanged);
}

void ActivitySelector::userChanged()
{
    m_changed = true;
}

bool ActivitySelector::refresh()
{
    int current;
    QList<QStringPair> activities = m_plugin->activities(&current);
    m_cmbActivity->clear();
    foreach (const QStringPair &activity, activities)
        m_cmbActivity->addItem(activity.second, activity.first);
    m_cmbActivity->setCurrentIndex(current);
    m_cmbActivity->updateGeometry();
    resize(size());
    return !activities.isEmpty();
}

void ActivitySelector::addKeep()
{
    m_cmbActivity->insertItem(0, tr("Keep item activity"), QLatin1String(Constants::KEEP_ACTIVITY));
    setActivity(QLatin1String(Constants::KEEP_ACTIVITY));
}

QString ActivitySelector::activity() const
{
    return m_cmbActivity->itemData(m_cmbActivity->currentIndex()).toString();
}

void ActivitySelector::setActivity(const QString &act)
{
    int index = m_cmbActivity->findData(act);
    if (index != -1) {
        disconnect(m_cmbActivity, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                   this, &ActivitySelector::userChanged);
        m_cmbActivity->setCurrentIndex(index);
        connect(m_cmbActivity, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &ActivitySelector::userChanged);
    }
}

void ActivitySelector::newActivity()
{
    if (m_plugin->newActivity())
        refresh();
}
