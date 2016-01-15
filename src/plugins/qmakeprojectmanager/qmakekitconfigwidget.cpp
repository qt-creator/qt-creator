/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmakekitconfigwidget.h"

#include "qmakekitinformation.h"

#include <utils/fileutils.h>

#include <QLineEdit>

namespace QmakeProjectManager {
namespace Internal {

QmakeKitConfigWidget::QmakeKitConfigWidget(ProjectExplorer::Kit *k, const ProjectExplorer::KitInformation *ki) :
    ProjectExplorer::KitConfigWidget(k, ki),
    m_lineEdit(new QLineEdit)
{
    refresh(); // set up everything according to kit
    m_lineEdit->setToolTip(toolTip());
    connect(m_lineEdit, &QLineEdit::textEdited, this, &QmakeKitConfigWidget::mkspecWasChanged);
}

QmakeKitConfigWidget::~QmakeKitConfigWidget()
{
    delete m_lineEdit;
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
} // namespace QmakeProjectManager
