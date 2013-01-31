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

#include "fakevimactions.h"
#include "fakevimhandler.h"

// Please do not add any direct dependencies to other Qt Creator code  here.
// Instead emit signals and let the FakeVimPlugin channel the information to
// Qt Creator. The idea is to keep this file here in a "clean" state that
// allows easy reuse with any QTextEdit or QPlainTextEdit derived class.


#include <utils/qtcassert.h>

#include <QDebug>
#include <QObject>
#include <QCoreApplication>

using namespace Utils;

///////////////////////////////////////////////////////////////////////
//
// FakeVimSettings
//
///////////////////////////////////////////////////////////////////////

namespace FakeVim {
namespace Internal {

FakeVimSettings::FakeVimSettings()
{}

FakeVimSettings::~FakeVimSettings()
{
    qDeleteAll(m_items);
}

void FakeVimSettings::insertItem(int code, SavedAction *item,
    const QString &longName, const QString &shortName)
{
    QTC_ASSERT(!m_items.contains(code), qDebug() << code << item->toString(); return);
    m_items[code] = item;
    if (!longName.isEmpty()) {
        m_nameToCode[longName] = code;
        m_codeToName[code] = longName;
    }
    if (!shortName.isEmpty())
        m_nameToCode[shortName] = code;
}

void FakeVimSettings::readSettings(QSettings *settings)
{
    foreach (SavedAction *item, m_items)
        item->readSettings(settings);
}

void FakeVimSettings::writeSettings(QSettings *settings)
{
    foreach (SavedAction *item, m_items)
        item->writeSettings(settings);
}

SavedAction *FakeVimSettings::item(int code)
{
    QTC_ASSERT(m_items.value(code, 0), qDebug() << "CODE: " << code; return 0);
    return m_items.value(code, 0);
}

SavedAction *FakeVimSettings::item(const QString &name)
{
    return m_items.value(m_nameToCode.value(name, -1), 0);
}

QString FakeVimSettings::trySetValue(const QString &name, const QString &value)
{
    int code = m_nameToCode.value(name, -1);
    if (code == -1)
        return FakeVimHandler::tr("Unknown option: %1").arg(name);
    if (code == ConfigTabStop || code == ConfigShiftWidth) {
        if (value.toInt() <= 0)
            return FakeVimHandler::tr("Argument must be positive: %1=%2")
                    .arg(name).arg(value);
    }
    SavedAction *act = item(code);
    if (!act)
        return FakeVimHandler::tr("Unknown option: %1").arg(name);
    act->setValue(value);
    return QString();
}

FakeVimSettings *theFakeVimSettings()
{
    static FakeVimSettings *instance = 0;
    if (instance)
        return instance;

    instance = new FakeVimSettings;

    typedef QLatin1String _;
    SavedAction *item = 0;

    const QString group = _("FakeVim");
    item = new SavedAction(instance);
    item->setText(QCoreApplication::translate("FakeVim::Internal",
        "Use Vim-style Editing"));
    item->setSettingsKey(group, _("UseFakeVim"));
    item->setCheckable(true);
    item->setValue(false);
    instance->insertItem(ConfigUseFakeVim, item);

    item = new SavedAction(instance);
    item->setText(QCoreApplication::translate("FakeVim::Internal",
        "Read .vimrc"));
    item->setSettingsKey(group, _("ReadVimRc"));
    item->setCheckable(true);
    item->setValue(false);
    instance->insertItem(ConfigReadVimRc, item);

    item = new SavedAction(instance);
    item->setValue(true);
    item->setDefaultValue(true);
    item->setSettingsKey(group, _("StartOfLine"));
    item->setCheckable(true);
    instance->insertItem(ConfigStartOfLine, item, _("startofline"), _("sol"));

    item = new SavedAction(instance);
    item->setDefaultValue(8);
    item->setSettingsKey(group, _("TabStop"));
    instance->insertItem(ConfigTabStop, item, _("tabstop"), _("ts"));

    item = new SavedAction(instance);
    item->setDefaultValue(false);
    item->setValue(false);
    item->setSettingsKey(group, _("SmartTab"));
    instance->insertItem(ConfigSmartTab, item, _("smarttab"), _("sta"));

    item = new SavedAction(instance);
    item->setDefaultValue(true);
    item->setValue(true);
    item->setSettingsKey(group, _("HlSearch"));
    item->setCheckable(true);
    instance->insertItem(ConfigHlSearch, item, _("hlsearch"), _("hls"));

    item = new SavedAction(instance);
    item->setDefaultValue(8);
    item->setSettingsKey(group, _("ShiftWidth"));
    instance->insertItem(ConfigShiftWidth, item, _("shiftwidth"), _("sw"));

    item = new SavedAction(instance);
    item->setDefaultValue(false);
    item->setValue(false);
    item->setSettingsKey(group, _("ExpandTab"));
    item->setCheckable(true);
    instance->insertItem(ConfigExpandTab, item, _("expandtab"), _("et"));

    item = new SavedAction(instance);
    item->setDefaultValue(false);
    item->setValue(false);
    item->setSettingsKey(group, _("AutoIndent"));
    item->setValue(false);
    item->setCheckable(true);
    instance->insertItem(ConfigAutoIndent, item, _("autoindent"), _("ai"));

    item = new SavedAction(instance);
    item->setDefaultValue(false);
    item->setValue(false);
    item->setSettingsKey(group, _("SmartIndent"));
    item->setValue(false);
    item->setCheckable(true);
    instance->insertItem(ConfigSmartIndent, item, _("smartindent"), _("si"));

    item = new SavedAction(instance);
    item->setDefaultValue(true);
    item->setValue(true);
    item->setSettingsKey(group, _("IncSearch"));
    item->setCheckable(true);
    instance->insertItem(ConfigIncSearch, item, _("incsearch"), _("is"));

    item = new SavedAction(instance);
    item->setDefaultValue(false);
    item->setValue(false);
    item->setSettingsKey(group, _("UseCoreSearch")); item->setCheckable(true);
    instance->insertItem(ConfigUseCoreSearch, item,
        _("usecoresearch"), _("ucs"));

    item = new SavedAction(instance);
    item->setDefaultValue(false);
    item->setValue(false);
    item->setSettingsKey(group, _("SmartCase")); item->setCheckable(true);
    item->setCheckable(true);
    instance->insertItem(ConfigSmartCase, item, _("smartcase"), _("scs"));

    item = new SavedAction(instance);
    item->setDefaultValue(true);
    item->setValue(true);
    item->setSettingsKey(group, _("WrapScan")); item->setCheckable(true);
    item->setCheckable(true);
    instance->insertItem(ConfigWrapScan, item, _("wrapscan"), _("ws"));

    item = new SavedAction(instance);
    item->setDefaultValue(_("indent,eol,start"));
    item->setSettingsKey(group, _("Backspace"));
    instance->insertItem(ConfigBackspace, item, _("backspace"), _("bs"));

    item = new SavedAction(instance);
    item->setDefaultValue(_("@,48-57,_,192-255,a-z,A-Z"));
    item->setSettingsKey(group, _("IsKeyword"));
    instance->insertItem(ConfigIsKeyword, item, _("iskeyword"), _("isk"));

    // Invented here.
    item = new SavedAction(instance);
    item->setDefaultValue(false);
    item->setValue(false);
    item->setSettingsKey(group, _("ShowMarks"));
    item->setCheckable(true);
    instance->insertItem(ConfigShowMarks, item, _("showmarks"), _("sm"));

    item = new SavedAction(instance);
    item->setDefaultValue(false);
    item->setValue(false);
    item->setSettingsKey(group, _("PassControlKey"));
    item->setCheckable(true);
    instance->insertItem(ConfigPassControlKey, item, _("passcontrolkey"), _("pck"));

    item = new SavedAction(instance);
    item->setDefaultValue(QString());
    item->setValue(QString());
    item->setSettingsKey(group, _("Clipboard"));
    item->setCheckable(true);
    instance->insertItem(ConfigClipboard, item, _("clipboard"), _("cb"));

    item = new SavedAction(instance);
    item->setDefaultValue(true);
    item->setValue(true);
    item->setSettingsKey(group, _("ShowCmd")); item->setCheckable(true);
    item->setCheckable(true);
    instance->insertItem(ConfigShowCmd, item, _("showcmd"), _("sc"));

    return instance;
}

SavedAction *theFakeVimSetting(int code)
{
    return theFakeVimSettings()->item(code);
}

} // namespace Internal
} // namespace FakeVim
