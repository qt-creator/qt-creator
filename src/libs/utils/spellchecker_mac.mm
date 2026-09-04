// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "spellchecker.h"

#include "qtcassert.h"

#include <QCoreApplication>
#include <QThread>

#include <AppKit/NSSpellChecker.h>
#include <Foundation/NSArray.h>
#include <Foundation/NSString.h>

namespace Utils {

class MacSpellChecker final : public SpellChecker
{
public:
    MacSpellChecker()
        : m_tag([NSSpellChecker uniqueSpellDocumentTag])
    {
        QTC_CHECK(isMainThread());
    }

    bool isAvailable() const final { return true; }

    QStringList availableLanguages() const final
    {
        QTC_ASSERT(isMainThread(), return {});
        QStringList languages;
        @autoreleasepool {
            for (NSString *language in [sharedChecker() availableLanguages])
                languages.append(QString::fromNSString(language));
        }
        return languages;
    }

    QString defaultLanguage() const final
    {
        QTC_ASSERT(isMainThread(), return {});
        @autoreleasepool {
            return QString::fromNSString([sharedChecker() language]);
        }
    }

    QStringList suggestions(const QString &word, const QString &language) const final
    {
        QTC_ASSERT(isMainThread(), return {});
        QStringList suggestions;
        @autoreleasepool {
            NSString *string = word.toNSString();
            NSArray<NSString *> *guesses = [sharedChecker() guessesForWordRange:NSMakeRange(0, [string length])
                                                                       inString:string
                                                                       language:nsLanguage(language)
                                                         inSpellDocumentWithTag:m_tag];
            for (NSString *guess in guesses)
                suggestions.append(QString::fromNSString(guess));
        }
        return suggestions;
    }

    void learnWord(const QString &word, const QString &language) final
    {
        QTC_ASSERT(isMainThread(), return);
        Q_UNUSED(language)
        @autoreleasepool {
            [sharedChecker() learnWord:word.toNSString()];
        }
        emit dictionaryChanged();
    }

    void ignoreWord(const QString &word, const QString &language) final
    {
        QTC_ASSERT(isMainThread(), return);
        Q_UNUSED(language)
        @autoreleasepool {
            [sharedChecker() ignoreWord:word.toNSString() inSpellDocumentWithTag:m_tag];
        }
        emit dictionaryChanged();
    }

protected:
    QList<Range> check(const QString &text, const QString &language) const final
    {
        QTC_ASSERT(isMainThread(), return {});
        QList<Range> ranges;
        @autoreleasepool {
            NSString *string = text.toNSString();
            NSString *checkLanguage = nsLanguage(language);
            const NSInteger length = [string length];
            NSInteger offset = 0;
            while (offset < length) {
                const NSRange misspelled = [sharedChecker() checkSpellingOfString:string
                                                                       startingAt:offset
                                                                         language:checkLanguage
                                                                             wrap:NO
                                                           inSpellDocumentWithTag:m_tag
                                                                        wordCount:nullptr];
                if (misspelled.location == NSNotFound || misspelled.length == 0)
                    break;
                ranges.append({int(misspelled.location), int(misspelled.length)});
                offset = misspelled.location + misspelled.length;
            }
        }
        return ranges;
    }

private:
    static bool isMainThread()
    {
        return QThread::currentThread() == QCoreApplication::instance()->thread();
    }

    static NSSpellChecker *sharedChecker() { return [NSSpellChecker sharedSpellChecker]; }

    static NSString *nsLanguage(const QString &language)
    {
        return language.isEmpty() ? nil : language.toNSString();
    }

    const NSInteger m_tag;
};

SpellChecker *SpellChecker::createNativeSpellChecker()
{
    return new MacSpellChecker;
}

} // namespace Utils
