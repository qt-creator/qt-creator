/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
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
