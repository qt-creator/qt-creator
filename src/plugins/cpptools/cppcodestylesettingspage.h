/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CPPCODESTYLESETTINGSPAGE_H
#define CPPCODESTYLESETTINGSPAGE_H

#include "cpptools_global.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <QtGui/QWidget>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QVariant>
#include <QtCore/QStringList>

#include "cppcodestylesettings.h"
#include "cppcodeformatter.h"

namespace TextEditor {
    class FontSettings;
    class TabSettings;
    class ICodeStylePreferences;
    class SnippetEditorWidget;
    class CodeStyleEditor;
}

namespace CppTools {

class CppCodeStylePreferences;

namespace Internal {

namespace Ui {
class CppCodeStyleSettingsPage;
}

class CppCodeStylePreferencesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CppCodeStylePreferencesWidget(QWidget *parent = 0);
    virtual ~CppCodeStylePreferencesWidget();

    void setCodeStyle(CppTools::CppCodeStylePreferences *codeStylePreferences);

    QString searchKeywords() const;

private slots:
    void decorateEditors(const TextEditor::FontSettings &fontSettings);
    void setVisualizeWhitespace(bool on);
    void slotTabSettingsChanged(const TextEditor::TabSettings &settings);
    void slotCodeStyleSettingsChanged();
    void updatePreview();
    void setTabSettings(const TextEditor::TabSettings &settings);
    void setCodeStyleSettings(const CppTools::CppCodeStyleSettings &settings, bool preview = true);
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *, bool preview = true);

private:
    CppCodeStyleSettings cppCodeStyleSettings() const;

    CppCodeStylePreferences *m_preferences;
    Ui::CppCodeStyleSettingsPage *m_ui;
    QList<TextEditor::SnippetEditorWidget *> m_previews;
    bool m_blockUpdates;
};


class CppCodeStyleSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit CppCodeStyleSettingsPage(QWidget *parent = 0);
    ~CppCodeStyleSettingsPage();

    virtual QString id() const;
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;
    virtual QIcon categoryIcon() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish() { }
    virtual bool matches(const QString &) const;

private:
    QString m_searchKeywords;
    CppCodeStylePreferences *m_pageCppCodeStylePreferences;
    QPointer<TextEditor::CodeStyleEditor> m_widget;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPCODESTYLESETTINGSPAGE_H
