// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "spellchecksettings.h"

#include "syntaxhighlighter.h"
#include "texteditortr.h"

#include <utils/algorithm.h>
#include <utils/spellchecker.h>

#include <QComboBox>
#include <QLocale>
#include <QStandardItem>

using namespace Utils;

namespace TextEditor {

SpellCheckSettings &spellCheckSettings()
{
    static SpellCheckSettings settings;
    return settings;
}

QString spellCheckLanguage()
{
    const QString language = spellCheckSettings().language();
    if (!language.isEmpty())
        return language;

    // Prose in an editor and in a submit message is written in English far more often
    // than in the language the machine is set to, which is what the platform answers
    // with.
    SpellChecker *checker = SpellChecker::instance();
    const QString platformLanguage = checker->defaultLanguage();
    if (platformLanguage.startsWith("en"))
        return platformLanguage;
    const QString english = Utils::findOrDefault(checker->availableLanguages(),
                                                 [](const QString &candidate) {
                                                     return candidate.startsWith("en");
                                                 });
    return english.isEmpty() ? platformLanguage : english;
}

QString editorSpellCheckLanguage()
{
    return spellCheckSettings().checkText() ? spellCheckLanguage() : QString();
}

void followSpellCheckSettings(SyntaxHighlighter *highlighter)
{
    const auto apply = [highlighter] {
        highlighter->setSpellCheckLanguage(editorSpellCheckLanguage());
        highlighter->setSpellCheckStrings(spellCheckSettings().checkStrings());
    };
    apply();
    QObject::connect(&spellCheckSettings(), &AspectContainer::changed, highlighter, apply);
}

void SpellCheckLanguageAspect::fixupComboBox(QComboBox *comboBox)
{
    comboBox->setMinimumContentsLength(20);
}

static QString languageDisplayName(const QString &language)
{
    const QLocale locale(QString(language).replace('-', '_'));
    if (locale.language() == QLocale::C)
        return language;
    const QString name = QLocale::languageToString(locale.language());
    if (!language.contains('_') && !language.contains('-'))
        return name;
    return QString("%1 (%2)").arg(name, QLocale::territoryToString(locale.territory()));
}

static void fillLanguageItems(const StringSelectionAspect::ResultCallback &callback)
{
    auto defaultItem = new QStandardItem(Tr::tr("<Default>"));
    defaultItem->setToolTip(Tr::tr("English, if a dictionary for it is installed, "
                                   "otherwise the system language."));
    defaultItem->setData(QString());
    QList<QStandardItem *> items{defaultItem};

    QList<QStandardItem *> languageItems;
    for (const QString &language : SpellChecker::instance()->availableLanguages()) {
        auto item = new QStandardItem(languageDisplayName(language));
        item->setData(language);
        languageItems.append(item);
    }
    Utils::sort(languageItems, [](QStandardItem *a, QStandardItem *b) {
        return a->text().localeAwareCompare(b->text()) < 0;
    });
    callback(items + languageItems);
}

SpellCheckSettings::SpellCheckSettings()
{
    setAutoApply(false);
    setSettingsGroup("textSpellCheckSettings");

    checkText.setSettingsKey("CheckText");
    checkText.setLabelText(Tr::tr("Check comments and text files"));
    checkText.setToolTip(Tr::tr("Marks misspelled words in the comments of source files and "
                                "in the text of files that are prose, such as Markdown."));

    checkStrings.setSettingsKey("CheckStrings");
    checkStrings.setLabelText(Tr::tr("Check string literals"));
    checkStrings.setToolTip(Tr::tr("Marks misspelled words in string literals as well."));
    checkStrings.setEnabler(&checkText);

    language.setSettingsKey("Language");
    language.setLabelText(Tr::tr("Language:"));
    language.setToolTip(Tr::tr("The dictionary to check against. It applies to submit "
                               "messages as well."));
    language.setFillCallback(fillLanguageItems);
    language.setComboBoxEditable(false);

    readSettings();
}

} // namespace TextEditor
