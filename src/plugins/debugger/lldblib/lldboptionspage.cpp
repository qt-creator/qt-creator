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

#include "lldboptionspage.h"
#include "debuggerconstants.h"
#include "debuggerinternalconstants.h"

#include <coreplugin/icore.h>

#include <QCoreApplication>
#include <QUrl>
#include <QTextStream>
#include <QMessageBox>
#include <QDesktopServices>

namespace Debugger {
namespace Internal {

LldbOptionsPageWidget::LldbOptionsPageWidget(QWidget *parent, QSettings *s_)
    : QWidget(parent)
    , s(s_)
{
    m_ui.setupUi(this);
    load();
}

void LldbOptionsPageWidget::save()
{
    s->beginGroup(QLatin1String("LLDB"));
    s->setValue(QLatin1String("enabled"), m_ui.enableLldb->isChecked ());
    s->setValue(QLatin1String("gdbEmu"), m_ui.gdbEmu->isChecked ());
    s->endGroup();
}

void LldbOptionsPageWidget::load()
{
    s->beginGroup(QLatin1String("LLDB"));
    m_ui.enableLldb->setChecked(s->value(QLatin1String("enabled"), false).toBool());
    m_ui.gdbEmu->setChecked(s->value(QLatin1String("gdbEmu"), true).toBool());
    s->endGroup();
}

// ---------- LldbOptionsPage
LldbOptionsPage::LldbOptionsPage()
{
    //    m_options->fromSettings(Core::ICore::settings());
    setId(QLatin1String("F.Lldb"));
    setDisplayName(tr("LLDB"));
    setCategory(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Debugger", Constants::DEBUGGER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QIcon(QLatin1String(Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON)));
}

QWidget *LldbOptionsPage::createPage(QWidget *parent)
{
    m_widget = new LldbOptionsPageWidget(parent, Core::ICore::settings());
    return m_widget;
}

void LldbOptionsPage::apply()
{
    if (!m_widget)
        return;
    m_widget->save();
}

void LldbOptionsPage::finish()
{
}

bool LldbOptionsPage::matches(const QString &s) const
{
    return QString(s.toLower()).contains("lldb");
}

void addLldbOptionPages(QList<Core::IOptionsPage *> *opts)
{
    opts->push_back(new LldbOptionsPage);
}


} // namespace Internal
} // namespace Debugger
