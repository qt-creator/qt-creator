/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "fakevimactions.h"

// Please do not add any direct dependencies to other Qt Creator code  here. 
// Instead emit signals and let the FakeVimPlugin channel the information to
// Qt Creator. The idea is to keep this file here in a "clean" state that
// allows easy reuse with any QTextEdit or QPlainTextEdit derived class.


#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>
#include <QtCore/QtAlgorithms>
#include <QtCore/QStack>

using namespace Core::Utils;

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
    if (!shortName.isEmpty()) {
        m_nameToCode[shortName] = code;
    }
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

FakeVimSettings *theFakeVimSettings()
{
    static FakeVimSettings *instance = 0;
    if (instance)
        return instance;

    instance = new FakeVimSettings;

    SavedAction *item = 0;

    item = new SavedAction(instance);
    item->setText(QObject::tr("Use vim-style editing"));
    item->setSettingsKey("FakeVim", "UseFakeVim");
    instance->insertItem(ConfigUseFakeVim, item);

    item = new SavedAction(instance);
    item->setDefaultValue(false);
    item->setSettingsKey("FakeVim", "StartOfLine");
    instance->insertItem(ConfigStartOfLine, item, "startofline", "sol");

    item = new SavedAction(instance);
    item->setDefaultValue(8);
    item->setSettingsKey("FakeVim", "TabStop");
    instance->insertItem(ConfigTabStop, item, "tabstop", "ts");

    item = new SavedAction(instance);
    item->setDefaultValue(false);
    item->setSettingsKey("FakeVim", "SmartTab");
    instance->insertItem(ConfigSmartTab, item, "smarttab", "sta");

    item = new SavedAction(instance);
    item->setDefaultValue(true);
    item->setSettingsKey("FakeVim", "HlSearch");
    instance->insertItem(ConfigHlSearch, item, "hlsearch", "hls");

    item = new SavedAction(instance);
    item->setDefaultValue(8);
    item->setSettingsKey("FakeVim", "ShiftWidth");
    instance->insertItem(ConfigShiftWidth, item, "shiftwidth", "sw");

    item = new SavedAction(instance);
    item->setDefaultValue(false);
    item->setSettingsKey("FakeVim", "ExpandTab");
    instance->insertItem(ConfigExpandTab, item, "expandtab", "et");

    item = new SavedAction(instance);
    item->setDefaultValue(false);
    item->setSettingsKey("FakeVim", "AutoIndent");
    instance->insertItem(ConfigAutoIndent, item, "autoindent", "ai");

    item = new SavedAction(instance);
    item->setDefaultValue("indent,eol,start");
    item->setSettingsKey("FakeVim", "Backspace");
    instance->insertItem(ConfigBackspace, item, "backspace", "bs");

    item = new SavedAction(instance);
    item->setText(QObject::tr("FakeVim properties..."));
    instance->insertItem(SettingsDialog, item);

    return instance;
}

SavedAction *theFakeVimSetting(int code)
{
    return theFakeVimSettings()->item(code);
}

} // namespace Internal
} // namespace FakeVim
