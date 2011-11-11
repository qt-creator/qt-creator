/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "lldboptionspage.h"
#include "debuggerconstants.h"

#include <coreplugin/icore.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>
#include <QtCore/QTextStream>
#include <QtGui/QMessageBox>
#include <QtGui/QDesktopServices>

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
//    m_options->fromSettings(Core::ICore::instance()->settings());
}

LldbOptionsPage::~LldbOptionsPage()
{
}

QString LldbOptionsPage::settingsId()
{
    return QLatin1String("F.Lldb");
}

QString LldbOptionsPage::displayName() const
{
    return tr("LLDB");
}

QString LldbOptionsPage::category() const
{
    return QLatin1String(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
}

QString LldbOptionsPage::displayCategory() const
{
    return QCoreApplication::translate("Debugger", Debugger::Constants::DEBUGGER_SETTINGS_TR_CATEGORY);
}

QIcon LldbOptionsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Debugger::Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

QWidget *LldbOptionsPage::createPage(QWidget *parent)
{
    m_widget = new LldbOptionsPageWidget(parent, Core::ICore::instance()->settings());
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
