/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Dmitry Savchenko.
** Copyright (c) 2010 Vasiliy Sorokin.
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

#include "optionspage.h"
#include "constants.h"
#include "optionsdialog.h"

#include <coreplugin/icore.h>

#include <QList>
#include <QIcon>
#include <QMessageBox>

Todo::Internal::OptionsDialog *some = 0;

namespace Todo {
namespace Internal {

OptionsPage::OptionsPage(const Settings &settings, QObject *parent) :
    IOptionsPage(parent),
    m_dialog(0)
{
    setSettings(settings);

    setId(QLatin1String("TodoSettings"));
    setDisplayName(tr("To-Do"));
    setCategory(QLatin1String("To-Do"));
    setDisplayCategory(tr("To-Do"));
    setCategoryIcon(QLatin1String(Constants::ICON_TODO));
}

void OptionsPage::setSettings(const Settings &settings)
{
    m_settings = settings;
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
    return searchKeyWord == QLatin1String("todo");
}

} // namespace Internal
} // namespace Todo

