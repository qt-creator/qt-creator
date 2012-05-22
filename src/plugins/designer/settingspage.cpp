/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "settingspage.h"
#include "designerconstants.h"
#include "formeditorw.h"

#include <extensionsystem/pluginmanager.h>
#if QT_VERSION >= 0x050000
#    include <QDesignerOptionsPageInterface>
#else
#    include "qt_private/abstractoptionspage_p.h"
#endif

#include <QCoreApplication>
#include <QIcon>

using namespace Designer::Internal;

SettingsPage::SettingsPage(QDesignerOptionsPageInterface *designerPage) :
    m_designerPage(designerPage), m_initialized(false)
{
    setId(m_designerPage->name());
    setDisplayName(m_designerPage->name());
    setCategory(QLatin1String(Designer::Constants::SETTINGS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("Designer",
        Designer::Constants::SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Designer::Constants::SETTINGS_CATEGORY_ICON));
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    m_initialized = true;
    return m_designerPage->createPage(parent);
}

void SettingsPage::apply()
{
    if (m_initialized)
        m_designerPage->apply();
}

void SettingsPage::finish()
{
    if (m_initialized)
        m_designerPage->finish();
}

SettingsPageProvider::SettingsPageProvider(QObject *parent)
    : IOptionsPageProvider(parent), m_initialized(false)
{
    setCategory(QLatin1String(Designer::Constants::SETTINGS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("Designer",
        Designer::Constants::SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Designer::Constants::SETTINGS_CATEGORY_ICON));
}

QList<Core::IOptionsPage *> SettingsPageProvider::pages() const
{
    if (!m_initialized) {
        // get options pages from designer
        m_initialized = true;
        FormEditorW::ensureInitStage(FormEditorW::RegisterPlugins);
    }
    return FormEditorW::instance()->optionsPages();
}
