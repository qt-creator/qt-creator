/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "detailsbutton.h"

using namespace Utils;

DetailsButton::DetailsButton(QWidget *parent)
#ifdef Q_OS_MAC
    : QPushButton(parent),
#else
    : QToolButton(parent),
#endif
    m_checked(false)
{
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacSmallSize);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
#else
    setCheckable(true);
#endif
    setText(tr("Show Details"));
    connect(this, SIGNAL(clicked()),
            this, SLOT(onClicked()));
}

void DetailsButton::onClicked()
{
    m_checked = !m_checked;
}

bool DetailsButton::isToggled()
{
    return m_checked;
}
