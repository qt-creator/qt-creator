/**************************************************************************
**
** Copyright (c) 2013 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

ActivitySelector::ActivitySelector(QWidget *parent) :
    QWidget(parent),
    m_plugin(ClearCasePlugin::instance()),
    m_changed(false)
{
    QTC_ASSERT(m_plugin->isUcm(), return);

    QHBoxLayout *hboxLayout = new QHBoxLayout(this);
    hboxLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *lblActivity = new QLabel(tr("Select &activity:"));
    lblActivity->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    hboxLayout->addWidget(lblActivity);

    m_cmbActivity = new QComboBox(this);
    m_cmbActivity->setMinimumSize(QSize(350, 0));
    hboxLayout->addWidget(m_cmbActivity);

    QString addText = tr("Add");
    if (!m_plugin->settings().autoAssignActivityName)
        addText.append(QLatin1String("..."));
    QToolButton *btnAdd = new QToolButton;
    btnAdd->setText(addText);
    hboxLayout->addWidget(btnAdd);

#ifndef QT_NO_SHORTCUT
    lblActivity->setBuddy(m_cmbActivity);
#endif // QT_NO_SHORTCUT

    connect(btnAdd, SIGNAL(clicked()), this, SLOT(newActivity()));

    refresh();
    connect(m_cmbActivity, SIGNAL(currentIndexChanged(int)), this, SLOT(userChanged()));
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
    foreach (QStringPair activity, activities)
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
        disconnect(m_cmbActivity, SIGNAL(currentIndexChanged(int)), this, SLOT(userChanged()));
        m_cmbActivity->setCurrentIndex(index);
        connect(m_cmbActivity, SIGNAL(currentIndexChanged(int)), this, SLOT(userChanged()));
    }
}

void ActivitySelector::newActivity()
{
    if (m_plugin->newActivity())
        refresh();
}
