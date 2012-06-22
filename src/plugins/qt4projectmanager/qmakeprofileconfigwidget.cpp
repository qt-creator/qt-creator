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

#include "qmakeprofileconfigwidget.h"

#include "qt4projectmanagerconstants.h"
#include "qmakeprofileinformation.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QHBoxLayout>
#include <QLineEdit>

namespace Qt4ProjectManager {
namespace Internal {

QmakeProfileConfigWidget::QmakeProfileConfigWidget(ProjectExplorer::Profile *p,
                                                   QWidget *parent) :
    ProjectExplorer::ProfileConfigWidget(parent),
    m_profile(p),
    m_lineEdit(new QLineEdit)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);

    m_lineEdit->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_lineEdit);

    discard(); // set up everything according to profile
    connect(m_lineEdit, SIGNAL(textEdited(QString)), this, SIGNAL(dirty()));
}

QString QmakeProfileConfigWidget::displayName() const
{
    return tr("Qt mkspec:");
}

void QmakeProfileConfigWidget::makeReadOnly()
{
    m_lineEdit->setEnabled(false);
}

void QmakeProfileConfigWidget::apply()
{
    QmakeProfileInformation::setMkspec(m_profile, Utils::FileName::fromString(m_lineEdit->text()));
}

void QmakeProfileConfigWidget::discard()
{
    m_lineEdit->setText(QmakeProfileInformation::mkspec(m_profile).toString());
}

bool QmakeProfileConfigWidget::isDirty() const
{
    return m_lineEdit->text() != QmakeProfileInformation::mkspec(m_profile).toString();
}

} // namespace Internal
} // namespace Qt4ProjectManager
