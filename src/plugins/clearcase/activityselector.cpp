// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "activityselector.h"

#include "clearcaseconstants.h"
#include "clearcaseplugin.h"
#include "clearcasesettings.h"
#include "clearcasetr.h"

#include <utils/qtcassert.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

namespace ClearCase::Internal {

ActivitySelector::ActivitySelector(QWidget *parent) : QWidget(parent)
{
    QTC_ASSERT(ClearCasePlugin::viewData().isUcm, return);

    auto hboxLayout = new QHBoxLayout(this);
    hboxLayout->setContentsMargins(0, 0, 0, 0);

    auto lblActivity = new QLabel(Tr::tr("Select &activity:"));
    lblActivity->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    hboxLayout->addWidget(lblActivity);

    m_cmbActivity = new QComboBox(this);
    m_cmbActivity->setMinimumSize(QSize(350, 0));
    hboxLayout->addWidget(m_cmbActivity);

    QString addText = Tr::tr("Add");
    if (!ClearCasePlugin::settings().autoAssignActivityName)
        addText.append(QLatin1String("..."));
    auto btnAdd = new QToolButton;
    btnAdd->setText(addText);
    hboxLayout->addWidget(btnAdd);

    lblActivity->setBuddy(m_cmbActivity);

    connect(btnAdd, &QToolButton::clicked, this, &ActivitySelector::newActivity);

    refresh();
    connect(m_cmbActivity, &QComboBox::currentIndexChanged, this, &ActivitySelector::userChanged);
}

void ActivitySelector::userChanged()
{
    m_changed = true;
}

bool ActivitySelector::refresh()
{
    int current;
    const QList<QStringPair> activities = ClearCasePlugin::activities(&current);
    m_cmbActivity->clear();
    for (const QStringPair &activity : activities)
        m_cmbActivity->addItem(activity.second, activity.first);
    m_cmbActivity->setCurrentIndex(current);
    m_cmbActivity->updateGeometry();
    resize(size());
    return !activities.isEmpty();
}

void ActivitySelector::addKeep()
{
    m_cmbActivity->insertItem(0, Tr::tr("Keep item activity"), QLatin1String(Constants::KEEP_ACTIVITY));
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
        disconnect(m_cmbActivity, &QComboBox::currentIndexChanged,
                   this, &ActivitySelector::userChanged);
        m_cmbActivity->setCurrentIndex(index);
        connect(m_cmbActivity, &QComboBox::currentIndexChanged,
                this, &ActivitySelector::userChanged);
    }
}

void ActivitySelector::newActivity()
{
    if (ClearCasePlugin::newActivity())
        refresh();
}

} // ClearCase::Internal
