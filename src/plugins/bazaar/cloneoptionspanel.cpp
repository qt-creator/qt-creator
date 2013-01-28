/**************************************************************************
**
** Copyright (c) 2013 Hugues Delorme
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
#include "cloneoptionspanel.h"
#include "ui_cloneoptionspanel.h"

#include <QDebug>

namespace Bazaar {
namespace Internal {

CloneOptionsPanel::CloneOptionsPanel(QWidget *parent)
    : QWidget(parent),
      m_ui(new Ui::CloneOptionsPanel)
{
    m_ui->setupUi(this);
}

CloneOptionsPanel::~CloneOptionsPanel()
{
    delete m_ui;
}

bool CloneOptionsPanel::isUseExistingDirectoryOptionEnabled() const
{
    return m_ui->useExistingDirCheckBox->isChecked();
}

bool CloneOptionsPanel::isStackedOptionEnabled() const
{
    return m_ui->stackedCheckBox->isChecked();
}

bool CloneOptionsPanel::isStandAloneOptionEnabled() const
{
    return m_ui->standAloneCheckBox->isChecked();
}

bool CloneOptionsPanel::isBindOptionEnabled() const
{
    return m_ui->bindCheckBox->isChecked();
}

bool CloneOptionsPanel::isSwitchOptionEnabled() const
{
    return m_ui->switchCheckBox->isChecked();
}

bool CloneOptionsPanel::isHardLinkOptionEnabled() const
{
    return m_ui->hardlinkCheckBox->isChecked();
}

bool CloneOptionsPanel::isNoTreeOptionEnabled() const
{
    return m_ui->noTreeCheckBox->isChecked();
}

QString CloneOptionsPanel::revision() const
{
    return m_ui->revisionLineEdit->text().simplified();
}

} // namespace Internal
} // namespace Bazaar
