// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../cppeditor_global.h"
#include "cppquickfixassistant.h"

#include <extensionsystem/iplugin.h>
#include <texteditor/quickfix.h>

#include <QVersionNumber>

#include <optional>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#endif

namespace CppEditor {
namespace Internal {

class CppQuickFixOperation
    : public TextEditor::QuickFixOperation,
      public Internal::CppQuickFixInterface
{
public:
    explicit CppQuickFixOperation(const CppQuickFixInterface &interface, int priority = -1)
        : QuickFixOperation(priority), CppQuickFixInterface(interface)
    {}
    ~CppQuickFixOperation() override;
};

void createCppQuickFixFactories();
void destroyCppQuickFixFactories();

} // namespace Internal

/*!
    The QuickFixFactory is responsible for generating QuickFixOperation s which are
    applicable to the given QuickFixState.

    A QuickFixFactory should not have any state -- it can be invoked multiple times
    for different QuickFixState objects to create the matching operations, before any
    of those operations are applied (or released).

    This way, a single factory can be used by multiple editors, and a single editor
    can have multiple QuickFixCollector objects for different parts of the code.
 */

class CPPEDITOR_EXPORT CppQuickFixFactory : public QObject
{
    Q_OBJECT

public:
    CppQuickFixFactory();
    ~CppQuickFixFactory() override;

    using QuickFixOperations = TextEditor::QuickFixOperations;

    void match(const Internal::CppQuickFixInterface &interface, QuickFixOperations &result);

    static const QList<CppQuickFixFactory *> &cppQuickFixFactories();

    std::optional<QVersionNumber> clangdReplacement() const { return m_clangdReplacement; }
    void setClangdReplacement(const QVersionNumber &version) { m_clangdReplacement = version; }

    template<class Factory> static void registerFactory()
    {
        new Factory;
#ifdef WITH_TESTS
        cppEditor()->addTestCreator(Factory::createTest);
#endif
    }

#ifdef WITH_TESTS
    template<class Factory, class Test> static void registerFactoryWithStandardTest()
    {
        new Factory;
        cppEditor()->addTestCreator([] {
            auto factory = std::make_unique<Factory>();
            factory->enableTestMode();
            return new Test(std::move(factory));
        });
    }
#endif

    void enableTestMode() { m_testMode = true; }

    static ExtensionSystem::IPlugin *cppEditor();

protected:
    bool testMode() const { return m_testMode; }

private:
    /*!
        Implement this function to match and create the appropriate
        CppQuickFixOperation objects.
        Make sure that the function is "cheap". Otherwise, since the match()
        functions are also called to generate context menu entries,
        the user might experience a delay opening the context menu.
     */
    virtual void doMatch(const Internal::CppQuickFixInterface &interface,
                         QuickFixOperations &result) = 0;

    std::optional<QVersionNumber> m_clangdReplacement;
    bool m_testMode = false;
};

} // namespace CppEditor

#ifdef WITH_TESTS
#define REGISTER_QUICKFIX_FACTORY_WITH_STANDARD_TEST(Factory) \
    CppQuickFixFactory::registerFactoryWithStandardTest<Factory, Factory##Test>()
#else
#define REGISTER_QUICKFIX_FACTORY_WITH_STANDARD_TEST(Factory) \
    CppQuickFixFactory::registerFactory<Factory>()
#endif
