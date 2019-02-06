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

QmakeKitAspectWidget::QmakeKitAspectWidget(ProjectExplorer::Kit *k, const ProjectExplorer::KitAspect *ki) :
    ProjectExplorer::KitAspectWidget(k, ki),
    m_lineEdit(new QLineEdit)
{
    refresh(); // set up everything according to kit
    m_lineEdit->setToolTip(toolTip());
    connect(m_lineEdit, &QLineEdit::textEdited, this, &QmakeKitAspectWidget::mkspecWasChanged);
}

QmakeKitAspectWidget::~QmakeKitAspectWidget()
{
    delete m_lineEdit;
}

QWidget *QmakeKitAspectWidget::mainWidget() const
{
    return m_lineEdit;
}

QString QmakeKitAspectWidget::displayName() const
{
    return tr("Qt mkspec");
}

QString QmakeKitAspectWidget::toolTip() const
{
    return tr("The mkspec to use when building the project with qmake.<br>"
              "This setting is ignored when using other build systems.");
}

void QmakeKitAspectWidget::makeReadOnly()
{
    m_lineEdit->setEnabled(false);
}

void QmakeKitAspectWidget::refresh()
{
    if (!m_ignoreChange)
        m_lineEdit->setText(QmakeKitAspect::mkspec(m_kit).toUserOutput());
}

void QmakeKitAspectWidget::mkspecWasChanged(const QString &text)
{
    m_ignoreChange = true;
    QmakeKitAspect::setMkspec(m_kit, Utils::FileName::fromString(text));
    m_ignoreChange = false;
}

} // namespace Internal
} // namespace QmakeProjectManager
