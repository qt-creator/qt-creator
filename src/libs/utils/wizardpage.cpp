// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "wizardpage.h"

#include "wizard.h"

/*!
  \class Utils::WizardPage
  \inmodule QtCreator

  \brief QWizardPage with a couple of improvements.

  Adds a way to register fields so that a Utils::Wizard can check
  whether those fields are actually defined and a new method
  that is called once the page was added to the wizard.
*/

namespace Utils {

WizardPage::WizardPage(QWidget *parent) : QWizardPage(parent)
{ }

void WizardPage::pageWasAdded()
{
    auto wiz = qobject_cast<Wizard *>(wizard());
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
    auto wiz = qobject_cast<Wizard *>(wizard());
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
