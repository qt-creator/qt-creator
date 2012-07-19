/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef FAKEVIM_ACTIONS_H
#define FAKEVIM_ACTIONS_H

#include <utils/savedaction.h>

#include <QHash>
#include <QObject>
#include <QString>

namespace FakeVim {
namespace Internal {

enum FakeVimSettingsCode
{
    ConfigUseFakeVim,
    ConfigReadVimRc,

    ConfigStartOfLine,
    ConfigHlSearch,
    ConfigTabStop,
    ConfigSmartTab,
    ConfigShiftWidth,
    ConfigExpandTab,
    ConfigAutoIndent,
    ConfigSmartIndent,
    ConfigIncSearch,
    ConfigUseCoreSearch,

    // indent  allow backspacing over autoindent
    // eol     allow backspacing over line breaks (join lines)
    // start   allow backspacing over the start of insert; CTRL-W and CTRL-U
    //         stop once at the start of insert.
    ConfigBackspace,

    // @,48-57,_,192-255
    ConfigIsKeyword,

    // other actions
    ConfigShowMarks,
    ConfigPassControlKey
};

class FakeVimSettings : public QObject
{
    Q_OBJECT

public:
    FakeVimSettings();
    ~FakeVimSettings();
    void insertItem(int code, Utils::SavedAction *item,
        const QString &longname = QString(),
        const QString &shortname = QString());

    Utils::SavedAction *item(int code);
    Utils::SavedAction *item(const QString &name);
    QString trySetValue(const QString &name, const QString &value);

    void readSettings(QSettings *settings);
    void writeSettings(QSettings *settings);

private:
    QHash<int, Utils::SavedAction *> m_items;
    QHash<QString, int> m_nameToCode;
    QHash<int, QString> m_codeToName;
};

FakeVimSettings *theFakeVimSettings();
Utils::SavedAction *theFakeVimSetting(int code);

} // namespace Internal
} // namespace FakeVim

#endif // FAKEVIM_ACTTIONS_H
