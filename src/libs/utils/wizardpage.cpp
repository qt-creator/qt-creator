/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    Wizard *wiz = qobject_cast<Wizard *>(wizard());
    if (wiz)
        wiz->registerFieldName(name);
    else
        m_toRegister.insert(name);

    registerField(name, widget, property, changedSignal);
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
