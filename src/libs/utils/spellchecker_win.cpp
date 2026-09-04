// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "spellchecker.h"

// The spell checking service exists since Windows 8. Tool chains that ship no
// headers for it fall back to the do-nothing implementation.
#if __has_include(<spellcheck.h>) && __has_include(<wrl/client.h>)

#include "qtcassert.h"

#include <QCoreApplication>
#include <QHash>
#include <QLocale>
#include <QThread>

// The precompiled header undefines CALLBACK, which ole2.h below needs.
#ifndef CALLBACK
#define CALLBACK __stdcall
#endif

#include <spellcheck.h>
#include <wrl/client.h>

namespace Utils {

using Microsoft::WRL::ComPtr;

static bool isMainThread()
{
    return QThread::currentThread() == QCoreApplication::instance()->thread();
}

static const wchar_t *nativeString(const QString &string)
{
    return reinterpret_cast<const wchar_t *>(string.utf16());
}

static QString takeString(LPWSTR string)
{
    const QString result = QString::fromWCharArray(string);
    CoTaskMemFree(string);
    return result;
}

static QStringList toStringList(const ComPtr<IEnumString> &strings)
{
    QStringList result;
    if (!strings)
        return result;
    LPOLESTR string = nullptr;
    while (strings->Next(1, &string, nullptr) == S_OK)
        result.append(takeString(string));
    return result;
}

class WindowsSpellChecker final : public SpellChecker
{
public:
    explicit WindowsSpellChecker(const ComPtr<ISpellCheckerFactory> &factory)
        : m_factory(factory)
    {}

    bool isAvailable() const final { return true; }

    QStringList availableLanguages() const final
    {
        QTC_ASSERT(isMainThread(), return {});
        ComPtr<IEnumString> languages;
        if (FAILED(m_factory->get_SupportedLanguages(&languages)))
            return {};
        return toStringList(languages);
    }

    QString defaultLanguage() const final
    {
        QTC_ASSERT(isMainThread(), return {});
        const QString language = QLocale::system().name().replace('_', '-');
        BOOL supported = FALSE;
        if (FAILED(m_factory->IsSupported(nativeString(language), &supported)) || !supported)
            return {};
        return language;
    }

    QStringList suggestions(const QString &word, const QString &language) const final
    {
        QTC_ASSERT(isMainThread(), return {});
        ISpellChecker *checker = checkerFor(language);
        if (!checker)
            return {};
        ComPtr<IEnumString> suggestions;
        if (FAILED(checker->Suggest(nativeString(word), &suggestions)))
            return {};
        return toStringList(suggestions);
    }

    void learnWord(const QString &word, const QString &language) final
    {
        QTC_ASSERT(isMainThread(), return);
        if (ISpellChecker *checker = checkerFor(language)) {
            checker->Add(nativeString(word));
            emit dictionaryChanged();
        }
    }

    void ignoreWord(const QString &word, const QString &language) final
    {
        QTC_ASSERT(isMainThread(), return);
        if (ISpellChecker *checker = checkerFor(language)) {
            checker->Ignore(nativeString(word));
            emit dictionaryChanged();
        }
    }

protected:
    QList<Range> check(const QString &text, const QString &language) const final
    {
        QTC_ASSERT(isMainThread(), return {});
        ISpellChecker *checker = checkerFor(language);
        if (!checker)
            return {};
        ComPtr<IEnumSpellingError> errors;
        if (FAILED(checker->Check(nativeString(text), &errors)) || !errors)
            return {};

        QList<Range> ranges;
        ComPtr<ISpellingError> error;
        while (errors->Next(&error) == S_OK && error) {
            ULONG start = 0;
            ULONG length = 0;
            if (SUCCEEDED(error->get_StartIndex(&start)) && SUCCEEDED(error->get_Length(&length)))
                ranges.append({int(start), int(length)});
            error.Reset();
        }
        return ranges;
    }

private:
    ISpellChecker *checkerFor(const QString &language) const
    {
        const QString tag = language.isEmpty() ? defaultLanguage() : language;
        if (tag.isEmpty())
            return nullptr;
        auto it = m_checkers.find(tag);
        if (it == m_checkers.end()) {
            ComPtr<ISpellChecker> checker;
            const HRESULT created = m_factory->CreateSpellChecker(nativeString(tag), &checker);
            QTC_CHECK(SUCCEEDED(created));
            it = m_checkers.insert(tag, checker);
        }
        // Not it->Get(): QHash::iterator::operator->() takes the address of the value,
        // and ComPtr::operator&() releases the interface.
        return it.value().Get();
    }

    const ComPtr<ISpellCheckerFactory> m_factory;
    mutable QHash<QString, ComPtr<ISpellChecker>> m_checkers;
};

SpellChecker *SpellChecker::createNativeSpellChecker()
{
    // The apartment is bound to the thread that gets here first, and everything the
    // checkers do afterwards has to happen on it.
    QTC_ASSERT(isMainThread(), return nullptr);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    ComPtr<ISpellCheckerFactory> factory;
    if (FAILED(CoCreateInstance(__uuidof(SpellCheckerFactory), nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&factory)))) {
        return nullptr;
    }
    return new WindowsSpellChecker(factory);
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
