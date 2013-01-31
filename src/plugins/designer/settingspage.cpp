/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
    setId(Core::Id::fromString(m_designerPage->name()));
    setDisplayName(m_designerPage->name());
    setCategory(Designer::Constants::SETTINGS_CATEGORY);
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
    setCategory(Designer::Constants::SETTINGS_CATEGORY);
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
