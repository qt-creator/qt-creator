/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef GENERALSETTINGSPAGE_H
#define GENERALSETTINGSPAGE_H

#include "texteditor_global.h"

#include <coreplugin/icore.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <QtCore/QObject>

namespace TextEditor {

struct TabSettings;
struct StorageSettings;
struct DisplaySettings;

struct TEXTEDITOR_EXPORT GeneralSettingsPageParameters {
    QString name;
    QString category;
    QString trCategory;
    QString settingsPrefix;
};

class Ui_generalSettingsPage;

class TEXTEDITOR_EXPORT GeneralSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    GeneralSettingsPage(Core::ICore *core,
                        const GeneralSettingsPageParameters &p,
                        QObject *parent);
    virtual ~GeneralSettingsPage();

    //IOptionsPage
    QString name() const;
    QString category() const;
    QString trCategory() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }

    TabSettings tabSettings() const;
    StorageSettings storageSettings() const;
    DisplaySettings displaySettings() const;

    void setDisplaySettings(const DisplaySettings &);

signals:
    void tabSettingsChanged(const TextEditor::TabSettings &);
    void storageSettingsChanged(const TextEditor::StorageSettings &);
    void displaySettingsChanged(const TextEditor::DisplaySettings &);

private:
    void settingsFromUI(TabSettings &rc,
                        StorageSettings &storageSettings,
                        DisplaySettings &displaySettings) const;
    void settingsToUI();
    struct GeneralSettingsPagePrivate;
    GeneralSettingsPagePrivate *m_d;
};

} // namespace TextEditor

#endif // GENERALSETTINGSPAGE_H
