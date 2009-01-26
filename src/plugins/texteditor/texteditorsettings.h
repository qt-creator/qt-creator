/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef TEXTEDITORSETTINGS_H
#define TEXTEDITORSETTINGS_H

#include "texteditor_global.h"

#include <QtCore/QObject>

namespace TextEditor {

class GeneralSettingsPage;
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
    TextEditor::FontSettingsPage *m_fontSettingsPage;
    TextEditor::GeneralSettingsPage *m_generalSettingsPage;

    static TextEditorSettings *m_instance;
};

} // namespace TextEditor

#endif // TEXTEDITORSETTINGS_H
