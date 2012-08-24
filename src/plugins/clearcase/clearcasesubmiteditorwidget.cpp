/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 AudioCodes Ltd.
**
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "activityselector.h"
#include "clearcasesubmiteditorwidget.h"

#include <vcsbase/submitfilemodel.h>

#include <QCheckBox>
#include <QFrame>
#include <QVBoxLayout>

using namespace ClearCase::Internal;

ClearCaseSubmitEditorWidget::ClearCaseSubmitEditorWidget(QWidget *parent) :
    Utils::SubmitEditorWidget(parent)
{
    setDescriptionMandatory(false);
    QWidget *checkInWidget = new QWidget(this);

    QVBoxLayout *verticalLayout = new QVBoxLayout(checkInWidget);
    m_actSelector = new ActivitySelector;
    verticalLayout->addWidget(m_actSelector);

    QFrame *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    verticalLayout->addWidget(line);

    m_chkIdentical = new QCheckBox(tr("Chec&k in even if identical to previous version"));
    verticalLayout->addWidget(m_chkIdentical);

    m_chkPTime = new QCheckBox(tr("&Preserve file modification time"));
    verticalLayout->addWidget(m_chkPTime);

    insertTopWidget(checkInWidget);
}

QString ClearCaseSubmitEditorWidget::activity() const
{
    return m_actSelector->activity();
}

bool ClearCaseSubmitEditorWidget::isIdentical() const
{
    return m_chkIdentical->isChecked();
}

bool ClearCaseSubmitEditorWidget::isPreserve() const
{
    return m_chkPTime->isChecked();
}

void ClearCaseSubmitEditorWidget::setActivity(const QString &act)
{
    m_actSelector->setActivity(act);
}

bool ClearCaseSubmitEditorWidget::activityChanged() const
{
    return m_actSelector->changed();
}

void ClearCaseSubmitEditorWidget::addKeep()
{
    m_actSelector->addKeep();
}

QString ClearCaseSubmitEditorWidget::commitName() const
{
    return tr("&Check In");
}
