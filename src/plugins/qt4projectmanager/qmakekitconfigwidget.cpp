/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmakekitconfigwidget.h"

#include "qt4projectmanagerconstants.h"
#include "qmakekitinformation.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QHBoxLayout>
#include <QLineEdit>

namespace Qt4ProjectManager {
namespace Internal {

QmakeKitConfigWidget::QmakeKitConfigWidget(ProjectExplorer::Kit *k, QWidget *parent) :
    ProjectExplorer::KitConfigWidget(parent),
    m_kit(k),
    m_lineEdit(new QLineEdit)
{
    setToolTip(tr("The mkspec to use when building the project with qmake.<br>"
                  "This setting is ignored when using other build systems."));
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);

    m_lineEdit->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_lineEdit);

    discard(); // set up everything according to kit
    connect(m_lineEdit, SIGNAL(textEdited(QString)), this, SIGNAL(dirty()));
}

QString QmakeKitConfigWidget::displayName() const
{
    return tr("Qt mkspec:");
}

void QmakeKitConfigWidget::makeReadOnly()
{
    m_lineEdit->setEnabled(false);
}

void QmakeKitConfigWidget::apply()
{
    QmakeKitInformation::setMkspec(m_kit, Utils::FileName::fromString(m_lineEdit->text()));
}

void QmakeKitConfigWidget::discard()
{
    m_lineEdit->setText(QmakeKitInformation::mkspec(m_kit).toString());
}

bool QmakeKitConfigWidget::isDirty() const
{
    return m_lineEdit->text() != QmakeKitInformation::mkspec(m_kit).toString();
}

} // namespace Internal
} // namespace Qt4ProjectManager
