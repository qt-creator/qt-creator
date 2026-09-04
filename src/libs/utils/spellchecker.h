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
