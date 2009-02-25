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

#ifndef TEXTEDITORSETTINGS_H
#define TEXTEDITORSETTINGS_H

#include "texteditor_global.h"

#include <QtCore/QObject>

namespace TextEditor {

class BehaviorSettingsPage;
class DisplaySettingsPage;
class FontSettingsPage;
class FontSettings;
struct TabSettings;
struct StorageSettings;
struct DisplaySettings;

/**
 * This class provides a central place for basic text editor settings. These
 * settings include font settings, tab settings, storage settings and display
 * settings.
 */
class TEXTEDITOR_EXPORT TextEditorSettings : public QObject
{
    Q_OBJECT

public:
    explicit TextEditorSettings(QObject *parent);
    ~TextEditorSettings();

    static TextEditorSettings *instance();

    FontSettings fontSettings() const;
    TabSettings tabSettings() const;
    StorageSettings storageSettings() const;
    DisplaySettings displaySettings() const;

signals:
    void fontSettingsChanged(const TextEditor::FontSettings &);
    void tabSettingsChanged(const TextEditor::TabSettings &);
    void storageSettingsChanged(const TextEditor::StorageSettings &);
    void displaySettingsChanged(const TextEditor::DisplaySettings &);

private:
    FontSettingsPage *m_fontSettingsPage;
    BehaviorSettingsPage *m_behaviorSettingsPage;
    DisplaySettingsPage *m_displaySettingsPage;

    static TextEditorSettings *m_instance;
};

} // namespace TextEditor

#endif // TEXTEDITORSETTINGS_H
