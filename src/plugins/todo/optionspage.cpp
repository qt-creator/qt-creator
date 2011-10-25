/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Dmitry Savchenko.
** Copyright (c) 2010 Vasiliy Sorokin.
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

#include "optionspage.h"
#include "constants.h"

#include <coreplugin/icore.h>

#include <QList>
#include <QMessageBox>

Todo::Internal::OptionsDialog *some = 0;

namespace Todo {
namespace Internal {

OptionsPage::OptionsPage(const Settings &settings, QObject *parent) :
    IOptionsPage(parent),
    m_dialog(0)
{
    setSettings(settings);
}

OptionsPage::~OptionsPage()
{
}

void OptionsPage::setSettings(const Settings &settings)
{
    m_settings = settings;
}

QString OptionsPage::id() const
{
    return "TodoSettings";
}

QString OptionsPage::trName() const
{
    return tr("To-Do");
}

QString OptionsPage::category() const
{
    return "To-Do";
}

QString OptionsPage::trCategory() const
{
    return tr("To-Do");
}

QString OptionsPage::displayName() const
{
    return trName();
}

QString OptionsPage::displayCategory() const
{
    return trCategory();
}

QIcon OptionsPage::categoryIcon() const
{
    return QIcon(Constants::ICON_TODO);
}


QWidget *OptionsPage::createPage(QWidget *parent)
{
    m_dialog = new OptionsDialog(parent);
    m_dialog->setSettings(m_settings);
    return m_dialog;
}

void OptionsPage::apply()
{
    Settings newSettings = m_dialog->settings();

    if (newSettings != m_settings) {
        m_settings = newSettings;
        emit settingsChanged(m_settings);
    }
}

void OptionsPage::finish()
{
}

bool OptionsPage::matches(const QString &searchKeyWord) const
{
    return searchKeyWord == QString("todo");
}

} // namespace Internal
} // namespace Todo

