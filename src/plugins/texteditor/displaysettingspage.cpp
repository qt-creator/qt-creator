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

#include "displaysettingspage.h"
#include "displaysettings.h"
#include "ui_displaysettingspage.h"

#include <coreplugin/icore.h>

#include <QSettings>
#include <QTextStream>

using namespace TextEditor;

struct DisplaySettingsPage::DisplaySettingsPagePrivate
{
    explicit DisplaySettingsPagePrivate(const DisplaySettingsPageParameters &p);

    const DisplaySettingsPageParameters m_parameters;
    Internal::Ui::DisplaySettingsPage *m_page;
    DisplaySettings m_displaySettings;
    QString m_searchKeywords;
};

DisplaySettingsPage::DisplaySettingsPagePrivate::DisplaySettingsPagePrivate
    (const DisplaySettingsPageParameters &p)
    : m_parameters(p), m_page(0)
{
    if (const QSettings *s = Core::ICore::settings()) {
        m_displaySettings.fromSettings(m_parameters.settingsPrefix, s);
    }
}

DisplaySettingsPage::DisplaySettingsPage(const DisplaySettingsPageParameters &p,
                                         QObject *parent)
  : TextEditorOptionsPage(parent),
    d(new DisplaySettingsPagePrivate(p))
{
    setId(p.id);
    setDisplayName(p.displayName);
}

DisplaySettingsPage::~DisplaySettingsPage()
{
    delete d;
}

QWidget *DisplaySettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    d->m_page = new Internal::Ui::DisplaySettingsPage;
    d->m_page->setupUi(w);
    settingsToUI();
    if (d->m_searchKeywords.isEmpty()) {
        QTextStream(&d->m_searchKeywords) << d->m_page->displayLineNumbers->text()
          << ' ' << d->m_page->highlightCurrentLine->text()
          << ' ' << d->m_page->displayFoldingMarkers->text()
          << ' ' << d->m_page->highlightBlocks->text()
          << ' ' << d->m_page->visualizeWhitespace->text()
          << ' ' << d->m_page->animateMatchingParentheses->text()
          << ' ' << d->m_page->enableTextWrapping->text()
          << ' ' << d->m_page->autoFoldFirstComment->text()
          << ' ' << d->m_page->centerOnScroll->text();
        d->m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

void DisplaySettingsPage::apply()
{
    if (!d->m_page) // page was never shown
        return;
    DisplaySettings newDisplaySettings;

    settingsFromUI(newDisplaySettings);
    setDisplaySettings(newDisplaySettings);
}

void DisplaySettingsPage::finish()
{
    if (!d->m_page) // page was never shown
        return;
    delete d->m_page;
    d->m_page = 0;
}

void DisplaySettingsPage::settingsFromUI(DisplaySettings &displaySettings) const
{
    displaySettings.m_displayLineNumbers = d->m_page->displayLineNumbers->isChecked();
    displaySettings.m_textWrapping = d->m_page->enableTextWrapping->isChecked();
    displaySettings.m_showWrapColumn = d->m_page->showWrapColumn->isChecked();
    displaySettings.m_wrapColumn = d->m_page->wrapColumn->value();
    displaySettings.m_visualizeWhitespace = d->m_page->visualizeWhitespace->isChecked();
    displaySettings.m_displayFoldingMarkers = d->m_page->displayFoldingMarkers->isChecked();
    displaySettings.m_highlightCurrentLine = d->m_page->highlightCurrentLine->isChecked();
    displaySettings.m_highlightBlocks = d->m_page->highlightBlocks->isChecked();
    displaySettings.m_animateMatchingParentheses = d->m_page->animateMatchingParentheses->isChecked();
    displaySettings.m_markTextChanges = d->m_page->markTextChanges->isChecked();
    displaySettings.m_autoFoldFirstComment = d->m_page->autoFoldFirstComment->isChecked();
    displaySettings.m_centerCursorOnScroll = d->m_page->centerOnScroll->isChecked();
}

void DisplaySettingsPage::settingsToUI()
{
    const DisplaySettings &displaySettings = d->m_displaySettings;
    d->m_page->displayLineNumbers->setChecked(displaySettings.m_displayLineNumbers);
    d->m_page->enableTextWrapping->setChecked(displaySettings.m_textWrapping);
    d->m_page->showWrapColumn->setChecked(displaySettings.m_showWrapColumn);
    d->m_page->wrapColumn->setValue(displaySettings.m_wrapColumn);
    d->m_page->visualizeWhitespace->setChecked(displaySettings.m_visualizeWhitespace);
    d->m_page->displayFoldingMarkers->setChecked(displaySettings.m_displayFoldingMarkers);
    d->m_page->highlightCurrentLine->setChecked(displaySettings.m_highlightCurrentLine);
    d->m_page->highlightBlocks->setChecked(displaySettings.m_highlightBlocks);
    d->m_page->animateMatchingParentheses->setChecked(displaySettings.m_animateMatchingParentheses);
    d->m_page->markTextChanges->setChecked(displaySettings.m_markTextChanges);
    d->m_page->autoFoldFirstComment->setChecked(displaySettings.m_autoFoldFirstComment);
    d->m_page->centerOnScroll->setChecked(displaySettings.m_centerCursorOnScroll);
}

const DisplaySettings &DisplaySettingsPage::displaySettings() const
{
    return d->m_displaySettings;
}

void DisplaySettingsPage::setDisplaySettings(const DisplaySettings &newDisplaySettings)
{
    if (newDisplaySettings != d->m_displaySettings) {
        d->m_displaySettings = newDisplaySettings;
        if (QSettings *s = Core::ICore::settings())
            d->m_displaySettings.toSettings(d->m_parameters.settingsPrefix, s);

        emit displaySettingsChanged(newDisplaySettings);
    }
}

bool DisplaySettingsPage::matches(const QString &s) const
{
    return d->m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
