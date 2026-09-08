// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "spellchecker.h"

// Neither KDE nor GNOME offers a spell checking service. Both reach the installed
// dictionaries through Enchant, which is loaded here at run time so that a machine
// without it falls back to the do-nothing implementation.
#if QT_CONFIG(library)

#include "qtcassert.h"

#include <QCoreApplication>
#include <QHash>
#include <QLibrary>
#include <QLocale>
#include <QThread>

#include <memory>
#include <optional>

#include <sys/types.h>

namespace Utils {

struct EnchantBroker;
struct EnchantDict;

static bool isMainThread()
{
    return QThread::currentThread() == QCoreApplication::instance()->thread();
}

static void collectLanguage(const char *languageTag, const char *, const char *, const char *,
                            void *userData)
{
    auto languages = static_cast<QStringList *>(userData);
    // A language shows up once per provider that offers it.
    const QString tag = QString::fromUtf8(languageTag);
    if (!languages->contains(tag))
        languages->append(tag);
}

class Enchant
{
public:
    bool load()
    {
        // Distributions ship the versioned library, the development package the link.
        for (const int version : {2, -1}) {
            m_library.setFileNameAndVersion("enchant-2", version);
            if (m_library.load())
                break;
        }
        if (!m_library.isLoaded())
            return false;

        resolve(&brokerInit, "enchant_broker_init");
        resolve(&brokerFree, "enchant_broker_free");
        resolve(&requestDict, "enchant_broker_request_dict");
        resolve(&freeDict, "enchant_broker_free_dict");
        resolve(&dictExists, "enchant_broker_dict_exists");
        resolve(&listDicts, "enchant_broker_list_dicts");
        resolve(&dictCheck, "enchant_dict_check");
        resolve(&dictSuggest, "enchant_dict_suggest");
        resolve(&freeStringList, "enchant_dict_free_string_list");
        resolve(&dictAdd, "enchant_dict_add");
        resolve(&dictAddToSession, "enchant_dict_add_to_session");

        return brokerInit && brokerFree && requestDict && freeDict && dictExists && listDicts
               && dictCheck && dictSuggest && freeStringList && dictAdd && dictAddToSession;
    }

    using DescribeFn = void (*)(const char *languageTag, const char *providerName,
                                const char *providerDescription, const char *providerFile,
                                void *userData);

    EnchantBroker *(*brokerInit)() = nullptr;
    void (*brokerFree)(EnchantBroker *) = nullptr;
    EnchantDict *(*requestDict)(EnchantBroker *, const char *) = nullptr;
    void (*freeDict)(EnchantBroker *, EnchantDict *) = nullptr;
    int (*dictExists)(EnchantBroker *, const char *) = nullptr;
    void (*listDicts)(EnchantBroker *, DescribeFn, void *) = nullptr;
    int (*dictCheck)(EnchantDict *, const char *, ssize_t) = nullptr;
    char **(*dictSuggest)(EnchantDict *, const char *, ssize_t, size_t *) = nullptr;
    void (*freeStringList)(EnchantDict *, char **) = nullptr;
    void (*dictAdd)(EnchantDict *, const char *, ssize_t) = nullptr;
    void (*dictAddToSession)(EnchantDict *, const char *, ssize_t) = nullptr;

private:
    template<typename Function>
    void resolve(Function *function, const char *name)
    {
        *function = reinterpret_cast<Function>(m_library.resolve(name));
    }

    QLibrary m_library;
};

class UnixSpellChecker final : public SpellChecker
{
public:
    UnixSpellChecker(std::unique_ptr<Enchant> enchant, EnchantBroker *broker)
        : m_enchant(std::move(enchant))
        , m_broker(broker)
    {}

    ~UnixSpellChecker()
    {
        for (EnchantDict *dict : std::as_const(m_dicts)) {
            if (dict)
                m_enchant->freeDict(m_broker, dict);
        }
        m_enchant->brokerFree(m_broker);
    }

    bool isAvailable() const final { return !availableLanguages().isEmpty(); }

    QStringList availableLanguages() const final
    {
        QTC_ASSERT(isMainThread(), return {});
        if (!m_languages) {
            QStringList languages;
            m_enchant->listDicts(m_broker, collectLanguage, &languages);
            m_languages = languages;
        }
        return *m_languages;
    }

    QString defaultLanguage() const final
    {
        QTC_ASSERT(isMainThread(), return {});
        const QString language = QLocale::system().name();
        if (m_enchant->dictExists(m_broker, language.toUtf8().constData()) == 0)
            return {};
        return language;
    }

    QStringList suggestions(const QString &word, const QString &language) const final
    {
        QTC_ASSERT(isMainThread(), return {});
        EnchantDict *dict = dictFor(language);
        if (!dict)
            return {};
        const QByteArray utf8 = word.toUtf8();
        size_t count = 0;
        char **suggestions = m_enchant->dictSuggest(dict, utf8.constData(), utf8.size(), &count);
        if (!suggestions)
            return {};

        QStringList result;
        result.reserve(qsizetype(count));
        for (size_t i = 0; i < count; ++i)
            result.append(QString::fromUtf8(suggestions[i]));
        m_enchant->freeStringList(dict, suggestions);
        return result;
    }

    void learnWord(const QString &word, const QString &language) final
    {
        QTC_ASSERT(isMainThread(), return);
        if (EnchantDict *dict = dictFor(language)) {
            const QByteArray utf8 = word.toUtf8();
            m_enchant->dictAdd(dict, utf8.constData(), utf8.size());
            emit dictionaryChanged();
        }
    }

    void ignoreWord(const QString &word, const QString &language) final
    {
        QTC_ASSERT(isMainThread(), return);
        if (EnchantDict *dict = dictFor(language)) {
            const QByteArray utf8 = word.toUtf8();
            m_enchant->dictAddToSession(dict, utf8.constData(), utf8.size());
            emit dictionaryChanged();
        }
    }

protected:
    // Enchant checks a single word, not a text, so the words are ours to find.
    QList<Range> check(const QString &text, const QString &language) const final
    {
        QTC_ASSERT(isMainThread(), return {});
        EnchantDict *dict = dictFor(language);
        if (!dict)
            return {};

        QList<Range> ranges;
        for (const Range &range : wordRanges(text)) {
            const QByteArray word = text.mid(range.start, range.length).toUtf8();
            if (m_enchant->dictCheck(dict, word.constData(), word.size()) > 0)
                ranges.append(range);
        }
        return ranges;
    }

private:
    EnchantDict *dictFor(const QString &language) const
    {
        const QString tag = language.isEmpty() ? defaultLanguage() : language;
        if (tag.isEmpty())
            return nullptr;
        auto it = m_dicts.find(tag);
        if (it == m_dicts.end())
            it = m_dicts.insert(tag, m_enchant->requestDict(m_broker, tag.toUtf8().constData()));
        return it.value();
    }

    const std::unique_ptr<Enchant> m_enchant;
    EnchantBroker *const m_broker;
    mutable std::optional<QStringList> m_languages;
    mutable QHash<QString, EnchantDict *> m_dicts;
};

SpellChecker *SpellChecker::createNativeSpellChecker()
{
    // A broker is not thread safe, and everything the dictionaries do afterwards
    // has to happen on the thread that gets here first.
    QTC_ASSERT(isMainThread(), return nullptr);

    auto enchant = std::make_unique<Enchant>();
    if (!enchant->load())
        return nullptr;
    EnchantBroker *broker = enchant->brokerInit();
    if (!broker)
        return nullptr;
    return new UnixSpellChecker(std::move(enchant), broker);
}

} // namespace Utils

#else

namespace Utils {

SpellChecker *SpellChecker::createNativeSpellChecker()
{
    return nullptr;
}

} // namespace Utils

#endif
