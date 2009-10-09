/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "trkoptionspage.h"
#include "trkoptionswidget.h"
#include "trkoptions.h"
#include "debuggerconstants.h"

#include <coreplugin/icore.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>

namespace Debugger {
namespace Internal {

const char * const TRK_SETTINGS_ID = QT_TRANSLATE_NOOP("Debugger::Internal::TrkOptionsPage", "Symbian Trk");

TrkOptionsPage::TrkOptionsPage(const TrkOptionsPtr &options) :
    m_options(options)
{
}

TrkOptionsPage::~TrkOptionsPage()
{
}

QString TrkOptionsPage::settingsId()
{
    return QLatin1String(TRK_SETTINGS_ID);
}

QString TrkOptionsPage::trName() const
{
    return tr(TRK_SETTINGS_ID);
}

QString TrkOptionsPage::category() const
{
    return QLatin1String(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
}

QString TrkOptionsPage::trCategory() const
{
    return QCoreApplication::translate("Debugger", Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
}

QWidget *TrkOptionsPage::createPage(QWidget *parent)
{
    if (!m_widget)
        m_widget = new TrkOptionsWidget(parent);
    m_widget->setTrkOptions(*m_options);
    return m_widget;
}

void TrkOptionsPage::apply()
{
    if (!m_widget)
        return;
    const TrkOptions newOptions = m_widget->trkOptions();
    if (newOptions == *m_options)
        return;
    *m_options = newOptions;
    m_options->toSettings(Core::ICore::instance()->settings());
}

void TrkOptionsPage::finish()
{
}

} // namespace Internal
} // namespace Designer
