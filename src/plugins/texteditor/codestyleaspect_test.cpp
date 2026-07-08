// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codestyleaspect_test.h"

#include "codestyleeditor.h"
#include "codestylepool.h"
#include "icodestylepreferences.h"
#include "icodestylepreferencesfactory.h"
#include "tabsettings.h"
#include "textindenter.h"

#include <utils/layoutbuilder.h>

#include <QSpinBox>
#include <QTest>
#include <QWidget>

using namespace Utils;

namespace TextEditor::Internal {

const char TEST_LANGUAGE_ID[] = "TextEditor.CodeStyleAspectTest";

// A minimal code style editor adopting the CodeStyleEditor dirty/changed
// contract: it edits a deferred TabSettings aspect and reports through it.
class TestCodeStyleEditor final : public CodeStyleEditor
{
public:
    explicit TestCodeStyleEditor(ICodeStylePreferences *codeStyle)
    {
        m_tabSettings.setAutoApply(false);
        m_tabSettings.setPreferences(codeStyle);
        connect(&m_tabSettings, &AspectContainer::volatileValueChanged,
                this, &CodeStyleEditor::changed);

        auto widget = new QWidget;
        Layouting::Column{&m_tabSettings}.attachTo(widget);
        addEditorWidget(widget);
    }

    void apply() final { m_tabSettings.apply(); }
    void cancel() final { m_tabSettings.cancel(); }
    bool isDirty() const final { return m_tabSettings.isDirty(); }

private:
    TabSettings m_tabSettings;
};

class TestCodeStyleFactory final : public ICodeStylePreferencesFactory
{
public:
    TestCodeStyleFactory()
        : ICodeStylePreferencesFactory(TEST_LANGUAGE_ID)
    {
        setDisplayName(QString("Test"));
        setIndenterCreator([](QTextDocument *doc) { return new PlainTextIndenter(doc); });
        setCodeStyleCreator([] {
            auto prefs = new ICodeStylePreferences;
            prefs->setSettingsSuffix("TestCodeStyle");
            return prefs;
        });
        setSettingsEditorCreator([](ICodeStylePreferences *codeStyle) {
            return new TestCodeStyleEditor(codeStyle);
        });
    }
};

const char LIVE_TEST_LANGUAGE_ID[] = "TextEditor.CodeStyleAspectTest.Live";

// A code style editor that writes edits straight through to its preferences
// (like the C++/QML editors), reporting them via changed() and leaving
// apply/cancel/isDirty to the hosting CodeStyleAspect and its page-local copy.
class LiveTestCodeStyleEditor final : public CodeStyleEditor
{
public:
    explicit LiveTestCodeStyleEditor(ICodeStylePreferences *codeStyle)
    {
        m_tabSettings.setPreferences(codeStyle);
        connect(codeStyle, &ICodeStylePreferences::currentTabSettingsChanged,
                this, &CodeStyleEditor::changed);
        connect(codeStyle, &ICodeStylePreferences::currentValueChanged,
                this, &CodeStyleEditor::changed);

        auto widget = new QWidget;
        Layouting::Column{&m_tabSettings}.attachTo(widget);
        addEditorWidget(widget);
    }

private:
    TabSettings m_tabSettings;
};

class LiveTestCodeStyleFactory final : public ICodeStylePreferencesFactory
{
public:
    LiveTestCodeStyleFactory()
        : ICodeStylePreferencesFactory(LIVE_TEST_LANGUAGE_ID)
    {
        setDisplayName(QString("Live Test"));
        setIndenterCreator([](QTextDocument *doc) { return new PlainTextIndenter(doc); });
        setCodeStyleCreator([] {
            auto prefs = new ICodeStylePreferences;
            prefs->setSettingsSuffix("LiveTestCodeStyle");
            return prefs;
        });
        setSettingsEditorCreator([](ICodeStylePreferences *codeStyle) {
            return new LiveTestCodeStyleEditor(codeStyle);
        });
    }
};

const char MODEL_TEST_LANGUAGE_ID[] = "TextEditor.CodeStyleAspectTest.Model";

// Exercises ICodeStylePreferencesFactory::setupCodeStyles(): a built-in style
// plus an editable global that delegates to it.
class ModelTestCodeStyleFactory final : public ICodeStylePreferencesFactory
{
public:
    ModelTestCodeStyleFactory()
        : ICodeStylePreferencesFactory(MODEL_TEST_LANGUAGE_ID)
    {
        setCodeStyleCreator([] {
            auto prefs = new ICodeStylePreferences;
            prefs->setSettingsSuffix("ModelTestCodeStyle");
            return prefs;
        });
        setGlobalCodeStyleId("ModelTestGlobal");
        setDefaultCodeStyleId("builtin");
        setBuiltInCodeStyles([this](CodeStylePool *pool) {
            m_builtin.setId("builtin");
            m_builtin.setReadOnly(true);
            TabSettingsData ts;
            ts.m_tabSize = 3;
            ts.m_indentSize = 3;
            m_builtin.setTabSettings(ts);
            pool->addCodeStyle(&m_builtin);
        });
        setupCodeStyles();
    }

private:
    ICodeStylePreferences m_builtin;
};

const char VALUE_TEST_LANGUAGE_ID[] = "TextEditor.CodeStyleAspectTest.Value";

// A value editor (the option C style): just the widgets editing the
// preferences live. The hosting CodeStyleAspect builds the selector and preview
// and owns the deferral and dirtiness, so the value editor has no
// apply/cancel/isDirty of its own.
class ValueTestCodeStyleWidget final : public QWidget
{
public:
    explicit ValueTestCodeStyleWidget(ICodeStylePreferences *codeStyle)
    {
        m_tabSettings.setPreferences(codeStyle);
        Layouting::Column{&m_tabSettings}.attachTo(this);
    }

private:
    TabSettings m_tabSettings;
};

class ValueTestCodeStyleFactory final : public ICodeStylePreferencesFactory
{
public:
    ValueTestCodeStyleFactory()
        : ICodeStylePreferencesFactory(VALUE_TEST_LANGUAGE_ID)
    {
        setDisplayName(QString("Value Test"));
        setIndenterCreator([](QTextDocument *doc) { return new PlainTextIndenter(doc); });
        setCodeStyleCreator([] {
            auto prefs = new ICodeStylePreferences;
            prefs->setSettingsSuffix("ValueTestCodeStyle");
            return prefs;
        });
        setValueEditorCreator([](ICodeStylePreferences *codeStyle) {
            return new ValueTestCodeStyleWidget(codeStyle);
        });
    }
};

class CodeStyleAspectTest final : public QObject
{
    Q_OBJECT

private:
    // Locates the tab-size spin box by its (distinctive) current value.
    static QSpinBox *spinBoxWithValue(const QWidget *root, int value)
    {
        const QList<QSpinBox *> boxes = root->findChildren<QSpinBox *>();
        for (QSpinBox *box : boxes) {
            if (box->value() == value)
                return box;
        }
        return nullptr;
    }

    static TabSettingsData makeTabSettings(int tabSize, int indentSize)
    {
        TabSettingsData data;
        data.m_tabSize = tabSize;
        data.m_indentSize = indentSize;
        return data;
    }

private slots:
    void testUnopenedPageIsNotDirty()
    {
        TestCodeStyleFactory factory;
        ICodeStylePreferences codeStyle;
        codeStyle.setTabSettings(makeTabSettings(11, 7));

        CodeStyleAspect aspect(&codeStyle, TEST_LANGUAGE_ID);
        // The layouter was never run, so nothing was edited.
        QVERIFY(!aspect.isDirty());
    }

    void testEditMakesDirtyAndApplyCommits()
    {
        TestCodeStyleFactory factory;
        ICodeStylePreferences codeStyle;
        codeStyle.setTabSettings(makeTabSettings(11, 7));

        CodeStyleAspect aspect(&codeStyle, TEST_LANGUAGE_ID);
        QWidget host;
        aspect.layouter()().attachTo(&host);

        QVERIFY(!aspect.isDirty());

        QSpinBox *tabSize = spinBoxWithValue(&host, 11);
        QVERIFY(tabSize);
        tabSize->setValue(13);

        QVERIFY(aspect.isDirty());
        // The edit is held in the aspect, not yet written to the real style.
        QCOMPARE(codeStyle.tabSettings().m_tabSize, 11);

        aspect.apply();
        QCOMPARE(codeStyle.tabSettings().m_tabSize, 13);
        QVERIFY(!aspect.isDirty());
    }

    void testCancelReverts()
    {
        TestCodeStyleFactory factory;
        ICodeStylePreferences codeStyle;
        codeStyle.setTabSettings(makeTabSettings(11, 7));

        CodeStyleAspect aspect(&codeStyle, TEST_LANGUAGE_ID);
        QWidget host;
        aspect.layouter()().attachTo(&host);

        QSpinBox *tabSize = spinBoxWithValue(&host, 11);
        QVERIFY(tabSize);
        tabSize->setValue(13);
        QVERIFY(aspect.isDirty());

        aspect.cancel();
        QVERIFY(!aspect.isDirty());
        QCOMPARE(codeStyle.tabSettings().m_tabSize, 11);
        QCOMPARE(tabSize->value(), 11);
    }

    // Same expectations, but with an editor that writes through immediately
    // (the C++/QML style): the CodeStyleAspect page-local copy provides the
    // deferral and the comparison-based dirtiness.
    void testLiveWriteEditor()
    {
        LiveTestCodeStyleFactory factory;
        ICodeStylePreferences codeStyle;
        codeStyle.setTabSettings(makeTabSettings(11, 7));

        CodeStyleAspect aspect(&codeStyle, LIVE_TEST_LANGUAGE_ID);
        QWidget host;
        aspect.layouter()().attachTo(&host);

        QVERIFY(!aspect.isDirty());

        QSpinBox *tabSize = spinBoxWithValue(&host, 11);
        QVERIFY(tabSize);
        tabSize->setValue(13);

        QVERIFY(aspect.isDirty());
        QCOMPARE(codeStyle.tabSettings().m_tabSize, 11);

        aspect.apply();
        QCOMPARE(codeStyle.tabSettings().m_tabSize, 13);
        QVERIFY(!aspect.isDirty());

        tabSize->setValue(17);
        QVERIFY(aspect.isDirty());
        aspect.cancel();
        QVERIFY(!aspect.isDirty());
        QCOMPARE(codeStyle.tabSettings().m_tabSize, 13);
    }

    // The option C path: the language supplies only a value editor, and the
    // CodeStyleAspect builds the selector and preview around it. Behaves like
    // the live-write editor: edits mark dirty, apply commits, cancel reverts.
    void testValueEditor()
    {
        ValueTestCodeStyleFactory factory;
        ICodeStylePreferences codeStyle;
        codeStyle.setTabSettings(makeTabSettings(11, 7));

        CodeStyleAspect aspect(&codeStyle, VALUE_TEST_LANGUAGE_ID);
        QWidget host;
        aspect.layouter()().attachTo(&host);

        QVERIFY(!aspect.isDirty());

        QSpinBox *tabSize = spinBoxWithValue(&host, 11);
        QVERIFY(tabSize);
        tabSize->setValue(13);

        QVERIFY(aspect.isDirty());
        // The edit went into the page-local copy, not the real style.
        QCOMPARE(codeStyle.tabSettings().m_tabSize, 11);

        aspect.apply();
        QCOMPARE(codeStyle.tabSettings().m_tabSize, 13);
        QVERIFY(!aspect.isDirty());

        tabSize->setValue(17);
        QVERIFY(aspect.isDirty());
        aspect.cancel();
        QVERIFY(!aspect.isDirty());
        QCOMPARE(codeStyle.tabSettings().m_tabSize, 13);
    }

    // A live-write editor while a (custom, editable) pool delegate is active.
    // The page edits a page-local copy of the delegate, so the shared pool
    // delegate stays untouched until apply(); cancel() must revert the edit.
    void testDelegateCancelReverts()
    {
        LiveTestCodeStyleFactory factory;
        CodeStylePool pool(&factory, LIVE_TEST_LANGUAGE_ID);
        pool.setTransient(true); // do not persist the test style to the settings
        ICodeStylePreferences *delegate = pool.createCodeStyle("Custom");
        const TabSettingsData original = makeTabSettings(5, 8);
        delegate->setTabSettings(original);

        ICodeStylePreferences codeStyle;
        codeStyle.setId("RealStyle");
        codeStyle.setDelegatingPool(&pool);
        codeStyle.setTabSettings(makeTabSettings(11, 7));
        codeStyle.setCurrentDelegate(delegate);

        CodeStyleAspect aspect(&codeStyle, LIVE_TEST_LANGUAGE_ID);
        QWidget host;
        aspect.layouter()().attachTo(&host);

        // The editor shows the active delegate's tab size (5).
        QSpinBox *tabSize = spinBoxWithValue(&host, 5);
        QVERIFY(tabSize);
        tabSize->setValue(9);

        // The edit is held by the page copy and not committed to the shared
        // pool delegate.
        QCOMPARE(delegate->tabSettings(), original);
        QVERIFY(aspect.isDirty());

        aspect.cancel();
        QVERIFY(!aspect.isDirty());
        QCOMPARE(delegate->tabSettings(), original);
    }

    // Verifies the factory builds the pool + global and registers them.
    void testFactorySetupCodeStyles()
    {
        ModelTestCodeStyleFactory factory;

        ICodeStylePreferences *global = factory.globalCodeStyle();
        QVERIFY(global);
        QVERIFY(factory.codeStylePool());

        ICodeStylePreferences *builtin = factory.codeStylePool()->codeStyle("builtin");
        QVERIFY(builtin);

        // The global delegates to the built-in and resolves its tab settings.
        QCOMPARE(global->currentDelegate(), builtin);
        QCOMPARE(global->currentTabSettings().m_tabSize, 3);

        // It is registered for the language.
        QCOMPARE(codeStyleForLanguage(MODEL_TEST_LANGUAGE_ID), global);
    }
};

QObject *createCodeStyleAspectTest()
{
    return new CodeStyleAspectTest;
}

} // namespace TextEditor::Internal

#include "codestyleaspect_test.moc"
