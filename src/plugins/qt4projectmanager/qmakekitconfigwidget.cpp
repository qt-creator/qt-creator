/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qmakekitconfigwidget.h"

#include "qt4projectmanagerconstants.h"
#include "qmakekitinformation.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QLineEdit>

namespace Qt4ProjectManager {
namespace Internal {

QmakeKitConfigWidget::QmakeKitConfigWidget(ProjectExplorer::Kit *k) :
    ProjectExplorer::KitConfigWidget(k),
    m_lineEdit(new QLineEdit),
    m_ignoreChange(false)
{
    refresh(); // set up everything according to kit
    connect(m_lineEdit, SIGNAL(textEdited(QString)), this, SLOT(mkspecWasChanged(QString)));
}

QWidget *QmakeKitConfigWidget::mainWidget() const
{
    return m_lineEdit;
}

QString QmakeKitConfigWidget::displayName() const
{
    return tr("Qt mkspec:");
}

QString QmakeKitConfigWidget::toolTip() const
{
    return tr("The mkspec to use when building the project with qmake.<br>"
              "This setting is ignored when using other build systems.");
}

void QmakeKitConfigWidget::makeReadOnly()
{
    m_lineEdit->setEnabled(false);
}

void QmakeKitConfigWidget::refresh()
{
    if (!m_ignoreChange)
        m_lineEdit->setText(QmakeKitInformation::mkspec(m_kit).toUserOutput());
}

void QmakeKitConfigWidget::mkspecWasChanged(const QString &text)
{
    m_ignoreChange = true;
    QmakeKitInformation::setMkspec(m_kit, Utils::FileName::fromString(text));
    m_ignoreChange = false;
}

} // namespace Internal
} // namespace Qt4ProjectManager
