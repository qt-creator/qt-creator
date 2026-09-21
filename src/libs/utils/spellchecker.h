// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QObject>
#include <QStringList>

namespace Utils {

// Access to the spell checking service of the operating system. The default
// implementation is the one used on platforms that provide no such service:
// it reports nothing as misspelled.
class QTCREATOR_UTILS_EXPORT SpellChecker : public QObject
{
    Q_OBJECT

public:
    class Range
    {
    public:
        int start = 0;
        int length = 0;
    };

    static SpellChecker *instance();

    virtual bool isAvailable() const;
    // language tags as the platform spells them, for example "en_US"
    virtual QStringList availableLanguages() const;
    virtual QString defaultLanguage() const;

    // Misspelled words in text, with tokens that are code rather than prose
    // removed. Positions are relative to the beginning of text.
    QList<Range> misspelledRanges(const QString &text, const QString &language) const;
    // The same for a text of which only prose holds prose. Only the part of text that
    // prose reaches goes to the dictionary, which is the point of saying so: a call
    // into the spell checking service of the platform is the expensive part. Whether a
    // word is prose or a token of code is still read off the whole of text, since it is
    // the characters next to a word that tell, and the range it sits in does not.
    QList<Range> misspelledRanges(const QString &text, const QList<Range> &prose,
                                  const QString &language) const;

    // The words of text, for the platforms whose service checks one word at a time.
    static QList<Range> wordRanges(const QString &text);

    virtual QStringList suggestions(const QString &word, const QString &language) const;
    virtual void learnWord(const QString &word, const QString &language);
    virtual void ignoreWord(const QString &word, const QString &language);

signals:
    void dictionaryChanged();

protected:
    virtual QList<Range> check(const QString &text, const QString &language) const;

private:
    static SpellChecker *createNativeSpellChecker();
};

} // namespace Utils
