/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "profilemanagerconfigwidget.h"

#include "profile.h"

#include <QHBoxLayout>
#include <QFileDialog>
#include <QFormLayout>
#include <QPushButton>
#include <QSizePolicy>

namespace ProjectExplorer {
namespace Internal {

ProfileManagerConfigWidget::ProfileManagerConfigWidget(Profile *p, QWidget *parent) :
    ProfileConfigWidget(parent),
    m_layout(new QFormLayout),
    m_iconButton(new QPushButton),
    m_profile(p)
{
    m_layout->setMargin(0);
    m_layout->setSpacing(6);

    m_iconButton->setMinimumSize(70, 70);
    m_iconButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QVBoxLayout *iconLayout = new QVBoxLayout;
    iconLayout->addWidget(m_iconButton);
    iconLayout->addStretch();

    QHBoxLayout *masterLayout = new QHBoxLayout(this);
    masterLayout->addLayout(iconLayout);
    masterLayout->setMargin(0);
    masterLayout->setSpacing(12);
    masterLayout->addLayout(m_layout);

    discard();

    connect(m_iconButton, SIGNAL(clicked()), this, SLOT(setIcon()));
}

QString ProfileManagerConfigWidget::displayName() const
{
    return tr("Profiles");
}

void ProfileManagerConfigWidget::apply()
{
    foreach (ProfileConfigWidget *w, m_widgets)
        w->apply();
    m_profile->setIconPath(m_iconPath);
}

void ProfileManagerConfigWidget::discard()
{
    foreach (ProfileConfigWidget *w, m_widgets)
        w->discard();
    m_iconButton->setIcon(m_profile->icon());
    m_iconPath = m_profile->iconPath();
}

bool ProfileManagerConfigWidget::isDirty() const
{
    foreach (ProfileConfigWidget *w, m_widgets)
        if (w->isDirty())
            return true;
    return m_profile->iconPath() != m_iconPath;
}

void ProfileManagerConfigWidget::addConfigWidget(ProjectExplorer::ProfileConfigWidget *widget)
{
    Q_ASSERT(widget);
    Q_ASSERT(!m_widgets.contains(widget));

    connect(widget, SIGNAL(dirty()), this, SIGNAL(dirty()));
    m_layout->addRow(widget->displayName(), widget);
    m_widgets.append(widget);
}

void ProfileManagerConfigWidget::makeReadOnly()
{
    foreach (ProfileConfigWidget *w, m_widgets)
        w->makeReadOnly();
    m_iconButton->setEnabled(false);
}

void ProfileManagerConfigWidget::setIcon()
{
    const QString path = QFileDialog::getOpenFileName(0, tr("Select Icon"), m_iconPath, tr("Images (*.png *.xpm *.jpg)"));
    if (path.isEmpty())
        return;

    const QIcon icon = QIcon(path);
    if (icon.isNull())
        return;

    m_iconButton->setIcon(icon);
    m_iconPath = path;
    emit dirty();
}

} // namespace Internal
} // namespace ProjectExplorer
