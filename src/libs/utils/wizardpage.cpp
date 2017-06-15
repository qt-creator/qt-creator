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

#include "wizardpage.h"

#include "wizard.h"

/*! \class Utils::WizardPage

  \brief QWizardPage with a couple of improvements

  Adds a way to register fields so that a Utils::Wizard can check
  whether those fields are actually defined and a new method
  that is called once the page was added to the wizard.
*/

namespace Utils {

WizardPage::WizardPage(QWidget *parent) : QWizardPage(parent)
{ }

void WizardPage::pageWasAdded()
{
    Wizard *wiz = qobject_cast<Wizard *>(wizard());
    if (!wiz)
        return;

    for (auto i = m_toRegister.constBegin(); i != m_toRegister.constEnd(); ++i)
        wiz->registerFieldName(*i);

    m_toRegister.clear();
}

void WizardPage::registerFieldWithName(const QString &name, QWidget *widget,
                                       const char *property, const char *changedSignal)
{
    registerFieldName(name);
    registerField(name, widget, property, changedSignal);
}

void WizardPage::registerFieldName(const QString &name)
{
    Wizard *wiz = qobject_cast<Wizard *>(wizard());
    if (wiz)
        wiz->registerFieldName(name);
    else
        m_toRegister.insert(name);
}

bool WizardPage::handleReject()
{
    return false;
}

bool WizardPage::handleAccept()
{
    return false;
}

} // namespace Utils
