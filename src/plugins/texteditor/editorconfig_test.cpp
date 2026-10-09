// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editorconfig_test.h"

#include "editorconfig.h"
#include "icodestylepreferences.h"
#include "marginsettings.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "textdocument.h"
#include "texteditor.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/filepath.h>
#include <utils/temporarydirectory.h>

#include <QDir>
#include <QScopeGuard>
#include <QTest>
#include <QToolBar>
#include <QToolButton>

using namespace Utils;

namespace TextEditor::Internal {

class EditorConfigTest final : public QObject
{
    Q_OBJECT

private slots:
    void testGlob_data();
    void testGlob();
    void testPropertiesForFile();
    void testCacheIsBounded();
    void testDerivedIndentation();
    void testOverlongEntries();
    void testDisabled();
    void testOpenAndSave();
    void testRename();
    void testSaveAs();
    void testSaveAsEncoding_data();
    void testSaveAsEncoding();
    void testSaveAsFails();
    void testSaveUntitled();
    void testRestoreConfiguredStorageSettings();
    void testEditorEncodingWins();
};

void EditorConfigTest::testGlob_data()
{
    QTest::addColumn<QString>("glob");
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("matches");

    QTest::newRow("suffix") << "*.cpp" << "main.cpp" << true;
    QTest::newRow("suffix in subdirectory") << "*.cpp" << "src/main.cpp" << true;
    QTest::newRow("other suffix") << "*.cpp" << "main.h" << false;
    QTest::newRow("suffix is anchored") << "*.cpp" << "main.cpp.orig" << false;
    QTest::newRow("dot is literal") << "a.b" << "aXb" << false;
    QTest::newRow("name in subdirectory") << "Makefile" << "sub/Makefile" << true;
    QTest::newRow("star") << "*" << "a/b.txt" << true;
    QTest::newRow("path") << "src/*.cpp" << "src/main.cpp" << true;
    QTest::newRow("path is anchored") << "src/*.cpp" << "lib/src/main.cpp" << false;
    QTest::newRow("leading slash") << "/src/*.cpp" << "src/main.cpp" << true;
    QTest::newRow("star stops at slash") << "src/*.cpp" << "src/sub/main.cpp" << false;
    QTest::newRow("double star") << "src/**.cpp" << "src/sub/main.cpp" << true;
    QTest::newRow("double star directory empty") << "a/**/z" << "a/z" << true;
    QTest::newRow("double star directories") << "a/**/z" << "a/b/c/z" << true;
    QTest::newRow("question mark") << "?.txt" << "a.txt" << true;
    QTest::newRow("question marks") << "??.txt" << "a.txt" << false;
    QTest::newRow("brackets") << "[ab].txt" << "b.txt" << true;
    QTest::newRow("negated brackets") << "[!ab].txt" << "b.txt" << false;
    QTest::newRow("negated brackets other") << "[!ab].txt" << "c.txt" << true;
    QTest::newRow("bracket range") << "[a-c].txt" << "b.txt" << true;
    QTest::newRow("bracket range outside") << "[a-c].txt" << "d.txt" << false;
    QTest::newRow("braces") << "*.{cpp,h}" << "x.h" << true;
    QTest::newRow("braces other") << "*.{cpp,h}" << "x.c" << false;
    QTest::newRow("nested braces") << "{a,{b,c}}.txt" << "c.txt" << true;
    QTest::newRow("single brace") << "{single}.txt" << "{single}.txt" << true;
    QTest::newRow("single brace no alternative") << "{single}.txt" << "single.txt" << false;
    QTest::newRow("numeric range") << "file{1..3}.txt" << "file2.txt" << true;
    QTest::newRow("numeric range outside") << "file{1..3}.txt" << "file4.txt" << false;
    QTest::newRow("negative numeric range") << "file{-1..3}.txt" << "file-1.txt" << true;
    QTest::newRow("escaped star") << "\\*.txt" << "*.txt" << true;
    QTest::newRow("escaped star literal") << "\\*.txt" << "a.txt" << false;
    QTest::newRow("leading double star") << "**/*.md" << "README.md" << true;
    QTest::newRow("leading double star subdirectory") << "**/*.md" << "doc/README.md" << true;
    QTest::newRow("leading slash double star") << "/**/z" << "z" << true;
    QTest::newRow("range in alternative not taken")
        << "{*.txt,log{1..3}.dat}" << "a.txt" << true;
    QTest::newRow("range in alternative taken") << "{*.txt,log{1..3}.dat}" << "log2.dat" << true;
    QTest::newRow("range in alternative outside")
        << "{*.txt,log{1..3}.dat}" << "log4.dat" << false;
    QTest::newRow("negated brackets trailing dash") << "[!a-].txt" << "b.txt" << true;
    QTest::newRow("negated brackets trailing dash excluded") << "[!a-].txt" << "-.txt" << false;
    QTest::newRow("negated brackets only dash") << "[!-].txt" << "..txt" << true;
    QTest::newRow("negated brackets leading dash") << "[!-a].txt" << "b.txt" << true;
    QTest::newRow("brackets trailing dash") << "[a-].txt" << "-.txt" << true;
    QTest::newRow("deeply nested braces")
        << QString(10000, '{') + 'a' + QString(10000, '}') << "a" << false;
}

void EditorConfigTest::testGlob()
{
    QFETCH(QString, glob);
    QFETCH(QString, path);
    QFETCH(bool, matches);

    QCOMPARE(editorConfigGlobMatches(glob, path), matches);
}

void EditorConfigTest::testCacheIsBounded()
{
    TemporaryDirectory tempDir("qtc-editorconfig-test");
    QVERIFY(tempDir.isValid());
    QVERIFY(tempDir.path().pathAppended(".editorconfig").writeFileContents("root = true\n"));

    // Each lookup caches the directory of the file and the root directory.
    for (int i = 0; i <= kMaxCachedEditorConfigDirectories; ++i)
        editorConfigPropertiesForFile(tempDir.path().pathAppended(QString("d%1/a.txt").arg(i)));
    QVERIFY(editorConfigCacheSize() <= kMaxCachedEditorConfigDirectories);
}

void EditorConfigTest::testPropertiesForFile()
{
    TemporaryDirectory tempDir("qtc-editorconfig-test");
    QVERIFY(tempDir.isValid());
    const FilePath outer = tempDir.path();
    const FilePath project = outer.pathAppended("project");
    const FilePath sub = project.pathAppended("sub");
    QVERIFY(sub.createDir());

    QVERIFY(outer.pathAppended(".editorconfig").writeFileContents("root = true\n[*]\ntab_width = 7\n"));
    QVERIFY(project.pathAppended(".editorconfig")
                .writeFileContents("root = true\n"
                                   "\n"
                                   "[*]\n"
                                   "indent_style = space\n"
                                   "indent_size = 4\n"
                                   "charset = utf-8\n"
                                   "\n"
                                   "[*.mk]\n"
                                   "indent_style = tab\n"));
    QVERIFY(sub.pathAppended(".editorconfig")
                .writeFileContents("# comment\n"
                                   "; comment\n"
                                   "[*.cpp]\n"
                                   "INDENT_SIZE = 2\n"
                                   "charset = unset\n"));

    const QMap<QString, QString> cpp = editorConfigPropertiesForFile(sub.pathAppended("a.cpp"));
    const QMap<QString, QString> expectedCpp{{"indent_style", "space"}, {"indent_size", "2"}};
    QCOMPARE(cpp, expectedCpp);

    const QMap<QString, QString> mk = editorConfigPropertiesForFile(project.pathAppended("b.mk"));
    const QMap<QString, QString> expectedMk{
        {"indent_style", "tab"}, {"indent_size", "4"}, {"charset", "utf-8"}};
    QCOMPARE(mk, expectedMk);

    const QMap<QString, QString> outside
        = editorConfigPropertiesForFile(outer.pathAppended("c.txt"));
    const QMap<QString, QString> expectedOutside{{"tab_width", "7"}};
    QCOMPARE(outside, expectedOutside);

    // A relative path is not looked up relative to the working directory.
    const QString currentDir = QDir::currentPath();
    const auto restoreCurrentDir = qScopeGuard([currentDir] { QDir::setCurrent(currentDir); });
    QVERIFY(QDir::setCurrent(project.toFSPathString()));
    QVERIFY(editorConfigPropertiesForFile(FilePath::fromString("sub/a.cpp")).isEmpty());
    QVERIFY(editorConfigPropertiesForFile(FilePath::fromString("../a.txt")).isEmpty());
}

void EditorConfigTest::testDerivedIndentation()
{
    TabSettingsData tabSettings(TabSettingsData::SpacesOnlyTabPolicy,
                                8,
                                4,
                                TabSettingsData::ContinuationAlignWithSpaces);

    const EditorConfigProperties tabs = EditorConfigProperties::fromProperties(
        {{"indent_style", "Tab"}});
    QCOMPARE(tabs.indentWithTabs, std::optional<bool>(true));
    QVERIFY(tabs.indentSizeIsTabWidth);
    TabSettingsData tabsApplied = tabSettings;
    tabs.applyTo(tabsApplied);
    QCOMPARE(tabsApplied.m_tabPolicy, TabSettingsData::TabsOnlyTabPolicy);
    QCOMPARE(tabsApplied.m_indentSize, 8);
    QVERIFY(!tabsApplied.m_autoDetect);

    const EditorConfigProperties indentOnly = EditorConfigProperties::fromProperties(
        {{"indent_size", "2"}});
    QCOMPARE(indentOnly.indentSize, std::optional<int>(2));
    QCOMPARE(indentOnly.tabWidth, std::optional<int>(2));

    const EditorConfigProperties indentTab = EditorConfigProperties::fromProperties(
        {{"indent_size", "tab"}, {"tab_width", "5"}});
    QCOMPARE(indentTab.indentSize, std::optional<int>(5));

    QVERIFY(EditorConfigProperties::fromProperties({}).isEmpty());
    QVERIFY(EditorConfigProperties::fromProperties({{"end_of_line", "cr"}}).isEmpty());
    QVERIFY(EditorConfigProperties::fromProperties({{"unknown", "true"}}).isEmpty());

    // Values outside what the tab and margin settings accept are ignored.
    QVERIFY(EditorConfigProperties::fromProperties({{"indent_size", "1000000000"}}).isEmpty());
    QVERIFY(EditorConfigProperties::fromProperties({{"tab_width", "21"}}).isEmpty());
    QVERIFY(EditorConfigProperties::fromProperties({{"max_line_length", "1000"}}).isEmpty());
    QCOMPARE(EditorConfigProperties::fromProperties({{"indent_size", "20"}}).indentSize,
             std::optional<int>(20));
}

void EditorConfigTest::testOverlongEntries()
{
    TemporaryDirectory tempDir("qtc-editorconfig-test");
    QVERIFY(tempDir.isValid());
    const QByteArray longName(5000, 'x');
    QVERIFY(tempDir.path()
                .pathAppended(".editorconfig")
                .writeFileContents("root = true\n"
                                   "[*]\n"
                                   "indent_size = 2\n"
                                   + longName + " = 1\n"
                                   "tab_width = " + longName + "\n"
                                   "[" + longName + "]\n"
                                   "indent_size = 3\n"));

    const QMap<QString, QString> properties
        = editorConfigPropertiesForFile(tempDir.path().pathAppended(QString::fromLatin1(longName)));
    const QMap<QString, QString> expected{{"indent_size", "2"}};
    QCOMPARE(properties, expected);
}

void EditorConfigTest::testDisabled()
{
    TemporaryDirectory tempDir("qtc-editorconfig-test");
    QVERIFY(tempDir.isValid());
    QVERIFY(tempDir.path()
                .pathAppended(".editorconfig")
                .writeFileContents("root = true\n[*]\nindent_size = 3\n"));
    const FilePath file = tempDir.path().pathAppended("a.txt");

    QVERIFY(!EditorConfigProperties::forFile(file).isEmpty());

    const bool wasEnabled = globalEditorConfigSettings().enabled();
    const auto restore = qScopeGuard(
        [wasEnabled] { globalEditorConfigSettings().enabled.setValue(wasEnabled); });
    globalEditorConfigSettings().enabled.setValue(false);
    QVERIFY(EditorConfigProperties::forFile(file).isEmpty());
}

void EditorConfigTest::testOpenAndSave()
{
    TemporaryDirectory tempDir("qtc-editorconfig-test");
    QVERIFY(tempDir.isValid());
    QVERIFY(tempDir.path()
                .pathAppended(".editorconfig")
                .writeFileContents("root = true\n"
                                   "[*.txt]\n"
                                   "indent_style = tab\n"
                                   "tab_width = 3\n"
                                   "end_of_line = crlf\n"
                                   "charset = latin1\n"
                                   "trim_trailing_whitespace = true\n"
                                   "insert_final_newline = true\n"
                                   "max_line_length = 66\n"));
    const FilePath file = tempDir.path().pathAppended("a.txt");
    QVERIFY(file.writeFileContents("\tx\n   a  \n\xe4"));

    Core::IEditor *editor = Core::EditorManager::openEditor(file);
    QVERIFY(editor);
    const auto closeEditor = qScopeGuard([editor] {
        Core::EditorManager::closeEditors({editor}, false);
    });
    auto textEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(textEditor);
    TextDocument *document = textEditor->textDocument();

    QCOMPARE(document->plainText(), QString("\tx\n   a  \n") + QChar(0xe4));
    QCOMPARE(document->encoding().name(), TextEncoding(TextEncoding::Latin1).name());
    QCOMPARE(document->lineTerminationMode(), TextFileFormat::CRLFLineTerminator);
    QCOMPARE(textEditor->editorWidget()->marginSettings().m_marginColumn, 66);

    // A reload reads the line endings from the file again, they still follow .editorconfig.
    QVERIFY(document->reload(Core::IDocument::FlagReload, Core::IDocument::TypeContents));
    QCOMPARE(document->lineTerminationMode(), TextFileFormat::CRLFLineTerminator);

    // An explicit margin change for the editor wins, the configured settings do not.
    TextEditorWidget *widget = textEditor->editorWidget();
    MarginSettingsData marginSettings = widget->marginSettings();
    marginSettings.m_marginColumn = 42;
    widget->setMarginSettings(marginSettings);
    QCOMPARE(widget->marginSettings().m_marginColumn, 42);
    widget->setConfiguredMarginSettings(marginSettings);
    QCOMPARE(widget->marginSettings().m_marginColumn, 66);

    const TabSettingsData tabSettings = document->tabSettings();
    QCOMPARE(tabSettings.m_tabPolicy, TabSettingsData::TabsOnlyTabPolicy);
    QCOMPARE(tabSettings.m_tabSize, 3);
    QCOMPARE(tabSettings.m_indentSize, 3);

    // A code style set later, for example from a project, does not override it.
    ICodeStylePreferences *originalCodeStyle = document->codeStyle();
    ICodeStylePreferences codeStyle;
    codeStyle.setTabSettings(TabSettingsData(TabSettingsData::SpacesOnlyTabPolicy,
                                             8,
                                             4,
                                             TabSettingsData::ContinuationAlignWithSpaces));
    document->setCodeStyle(&codeStyle);
    QCOMPARE(document->tabSettings().m_tabPolicy, TabSettingsData::TabsOnlyTabPolicy);
    QCOMPARE(document->tabSettings().m_indentSize, 3);
    document->setCodeStyle(originalCodeStyle);

    StorageSettingsData storageSettings;
    storageSettings.m_cleanWhitespace = false;
    storageSettings.m_addFinalNewLine = false;
    document->setStorageSettings(storageSettings);

    QVERIFY(Core::DocumentManager::saveDocument(document));
    const Result<QByteArray> contents = file.fileContents();
    QVERIFY(contents);
    // Only trailing whitespace goes, the indentation is not rewritten to tabs.
    QCOMPARE(*contents, QByteArray("\tx\r\n   a\r\n\xe4\r\n"));
}

void EditorConfigTest::testRename()
{
    TemporaryDirectory tempDir("qtc-editorconfig-test");
    QVERIFY(tempDir.isValid());
    QVERIFY(tempDir.path()
                .pathAppended(".editorconfig")
                .writeFileContents("root = true\n"
                                   "[*]\n"
                                   "trim_trailing_whitespace = true\n"
                                   "max_line_length = 66\n"
                                   "[b.txt]\n"
                                   "trim_trailing_whitespace = false\n"
                                   "max_line_length = 77\n"));
    const FilePath file = tempDir.path().pathAppended("a.txt");
    QVERIFY(file.writeFileContents("x\n"));

    Core::IEditor *editor = Core::EditorManager::openEditor(file);
    QVERIFY(editor);
    const auto closeEditor = qScopeGuard([editor] {
        Core::EditorManager::closeEditors({editor}, false);
    });
    auto textEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(textEditor);
    TextDocument *document = textEditor->textDocument();
    TextEditorWidget *widget = textEditor->editorWidget();

    StorageSettingsData storageSettings;
    storageSettings.m_cleanWhitespace = false;
    storageSettings.m_addFinalNewLine = false;
    document->setStorageSettings(storageSettings);
    QVERIFY(document->storageSettings().m_cleanWhitespace);
    QCOMPARE(widget->marginSettings().m_marginColumn, 66);

    const FilePath renamed = tempDir.path().pathAppended("b.txt");
    QVERIFY(file.renameFile(renamed));
    Core::DocumentManager::renamedFile(file, renamed);
    QCOMPARE(document->filePath(), renamed);

    QVERIFY(!document->storageSettings().m_cleanWhitespace);
    QCOMPARE(widget->marginSettings().m_marginColumn, 77);

    document->document()->setPlainText("x  \n");
    QVERIFY(Core::DocumentManager::saveDocument(document));
    const Result<QByteArray> contents = renamed.fileContents();
    QVERIFY(contents);
    QCOMPARE(*contents, QByteArray("x  \n"));
}

static FilePath createDirWithEditorConfig(const FilePath &dir, const QByteArray &contents)
{
    if (!dir.createDir() || !dir.pathAppended(".editorconfig").writeFileContents(contents))
        return {};
    return dir;
}

void EditorConfigTest::testSaveAs()
{
    TemporaryDirectory tempDir("qtc-editorconfig-test");
    QVERIFY(tempDir.isValid());
    const FilePath source = createDirWithEditorConfig(tempDir.path().pathAppended("source"),
                                                      "root = true\n");
    QVERIFY(!source.isEmpty());
    const FilePath target = createDirWithEditorConfig(tempDir.path().pathAppended("target"),
                                                      "root = true\n"
                                                      "[*]\n"
                                                      "trim_trailing_whitespace = true\n"
                                                      "insert_final_newline = true\n");
    QVERIFY(!target.isEmpty());
    const FilePath file = source.pathAppended("a.txt");
    QVERIFY(file.writeFileContents("x  "));

    Core::IEditor *editor = Core::EditorManager::openEditor(file);
    QVERIFY(editor);
    const auto closeEditor = qScopeGuard([editor] {
        Core::EditorManager::closeEditors({editor}, false);
    });
    auto textEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(textEditor);
    TextDocument *document = textEditor->textDocument();
    TextEditorWidget *widget = textEditor->editorWidget();

    StorageSettingsData storageSettings;
    storageSettings.m_cleanWhitespace = false;
    storageSettings.m_addFinalNewLine = false;
    document->setStorageSettings(storageSettings);
    document->setLineTerminationMode(TextFileFormat::LFLineTerminator);
    MarginSettingsData marginSettings = widget->marginSettings();
    marginSettings.m_marginColumn = 42;
    widget->setMarginSettings(marginSettings);
    TabSettingsData tabSettings = document->tabSettings();
    tabSettings.m_autoDetect = false;
    tabSettings.m_tabPolicy = TabSettingsData::TabsOnlyTabPolicy;
    tabSettings.m_indentSize = 3;
    document->setTabSettings(tabSettings);

    // The first save at the new location already follows its .editorconfig.
    const FilePath saved = target.pathAppended("a.txt");
    QVERIFY(Core::DocumentManager::saveDocument(document, saved));
    QCOMPARE(document->filePath(), saved);
    const Result<QByteArray> contents = saved.fileContents();
    QVERIFY(contents);
    QCOMPARE(*contents, QByteArray("x\n"));

    // Neither .editorconfig sets a margin or indentation, so the explicit ones stay.
    QCOMPARE(widget->marginSettings().m_marginColumn, 42);
    QCOMPARE(document->tabSettings().m_tabPolicy, TabSettingsData::TabsOnlyTabPolicy);
    QCOMPARE(document->tabSettings().m_indentSize, 3);
}

void EditorConfigTest::testSaveAsEncoding_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QByteArray>("expected");
    QTest::addColumn<bool>("fromEditorConfig");

    QTest::newRow("encodable") << QString("a") + QChar(0xe4) + "\nb" << QByteArray("a\xe4\r\nb")
                               << true;
    // The euro sign has no Latin-1 representation, so the encoding stays UTF-8.
    QTest::newRow("not encodable") << QString("a") + QChar(0x20ac) + "\nb"
                                   << QByteArray("a\xe2\x82\xac\r\nb") << false;
}

void EditorConfigTest::testSaveAsEncoding()
{
    QFETCH(QString, text);
    QFETCH(QByteArray, expected);
    QFETCH(bool, fromEditorConfig);

    TemporaryDirectory tempDir("qtc-editorconfig-test");
    QVERIFY(tempDir.isValid());
    const FilePath source = createDirWithEditorConfig(tempDir.path().pathAppended("source"),
                                                      "root = true\n"
                                                      "[*]\n"
                                                      "charset = utf-8\n"
                                                      "end_of_line = lf\n");
    QVERIFY(!source.isEmpty());
    const FilePath target = createDirWithEditorConfig(tempDir.path().pathAppended("target"),
                                                      "root = true\n"
                                                      "[*]\n"
                                                      "charset = latin1\n"
                                                      "end_of_line = crlf\n");
    QVERIFY(!target.isEmpty());
    const FilePath file = source.pathAppended("a.txt");
    QVERIFY(file.writeFileContents(text.toUtf8()));

    Core::IEditor *editor = Core::EditorManager::openEditor(file);
    QVERIFY(editor);
    const auto closeEditor = qScopeGuard([editor] {
        Core::EditorManager::closeEditors({editor}, false);
    });
    auto textEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(textEditor);
    TextDocument *document = textEditor->textDocument();
    QCOMPARE(document->plainText(), text);

    // Without a cleanup the document stays unmodified, so no modificationChanged() refreshes
    // the tool bar.
    StorageSettingsData storageSettings;
    storageSettings.m_cleanWhitespace = false;
    storageSettings.m_addFinalNewLine = false;
    document->setStorageSettings(storageSettings);

    const FilePath saved = target.pathAppended("a.txt");
    QVERIFY(Core::DocumentManager::saveDocument(document, saved));
    const Result<QByteArray> contents = saved.fileContents();
    QVERIFY(contents);
    QCOMPARE(*contents, expected);
    QCOMPARE(document->lineTerminationMode(), TextFileFormat::CRLFLineTerminator);
    QCOMPARE(document->editorConfig().encoding.has_value(), fromEditorConfig);

    const QToolBar *toolBar = textEditor->editorWidget()->toolBar();
    QStringList labels;
    for (const QToolButton *button : toolBar->findChildren<QToolButton *>())
        labels << button->text();
    QVERIFY2(labels.contains("CRLF"), qPrintable(labels.join(", ")));
    QVERIFY2(labels.contains(document->encoding().displayName()), qPrintable(labels.join(", ")));
}

void EditorConfigTest::testSaveAsFails()
{
    TemporaryDirectory tempDir("qtc-editorconfig-test");
    QVERIFY(tempDir.isValid());
    const FilePath source = createDirWithEditorConfig(tempDir.path().pathAppended("source"),
                                                      "root = true\n"
                                                      "[*]\n"
                                                      "charset = utf-8\n");
    QVERIFY(!source.isEmpty());
    const FilePath target = createDirWithEditorConfig(tempDir.path().pathAppended("target"),
                                                      "root = true\n"
                                                      "[*]\n"
                                                      "indent_size = 2\n"
                                                      "charset = latin1\n"
                                                      "end_of_line = crlf\n"
                                                      "trim_trailing_whitespace = true\n");
    QVERIFY(!target.isEmpty());
    const FilePath file = source.pathAppended("a.txt");
    QVERIFY(file.writeFileContents("x  "));

    Core::IEditor *editor = Core::EditorManager::openEditor(file);
    QVERIFY(editor);
    const auto closeEditor = qScopeGuard([editor] {
        Core::EditorManager::closeEditors({editor}, false);
    });
    auto textEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(textEditor);
    TextDocument *document = textEditor->textDocument();

    StorageSettingsData storageSettings;
    storageSettings.m_cleanWhitespace = false;
    document->setStorageSettings(storageSettings);
    document->setLineTerminationMode(TextFileFormat::LFLineTerminator);
    TabSettingsData tabSettings = document->tabSettings();
    tabSettings.m_autoDetect = false;
    tabSettings.m_tabPolicy = TabSettingsData::TabsOnlyTabPolicy;
    tabSettings.m_indentSize = 3;
    document->setTabSettings(tabSettings);

    // A directory in place of the file makes the write fail.
    const FilePath blocked = target.pathAppended("a.txt");
    QVERIFY(blocked.createDir());
    QVERIFY(!document->save(blocked));

    QCOMPARE(document->filePath(), file);
    QCOMPARE(document->plainText(), QString("x  "));
    QCOMPARE(document->tabSettings().m_tabPolicy, TabSettingsData::TabsOnlyTabPolicy);
    QCOMPARE(document->tabSettings().m_indentSize, 3);
    QVERIFY(document->encoding().isUtf8());
    QCOMPARE(document->lineTerminationMode(), TextFileFormat::LFLineTerminator);
    QVERIFY(!document->storageSettings().m_cleanWhitespace);
}

void EditorConfigTest::testRestoreConfiguredStorageSettings()
{
    TemporaryDirectory tempDir("qtc-editorconfig-test");
    QVERIFY(tempDir.isValid());
    const FilePath source = createDirWithEditorConfig(tempDir.path().pathAppended("source"),
                                                      "root = true\n"
                                                      "[*]\n"
                                                      "insert_final_newline = true\n");
    QVERIFY(!source.isEmpty());
    const FilePath target = createDirWithEditorConfig(tempDir.path().pathAppended("target"),
                                                      "root = true\n");
    QVERIFY(!target.isEmpty());
    const FilePath file = source.pathAppended("a.txt");
    QVERIFY(file.writeFileContents("x"));

    Core::IEditor *editor = Core::EditorManager::openEditor(file);
    QVERIFY(editor);
    const auto closeEditor = qScopeGuard([editor] {
        Core::EditorManager::closeEditors({editor}, false);
    });
    auto textEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(textEditor);
    TextDocument *document = textEditor->textDocument();

    StorageSettingsData storageSettings;
    storageSettings.m_cleanWhitespace = false;
    storageSettings.m_addFinalNewLine = false;
    document->setStorageSettings(storageSettings);
    QVERIFY(document->storageSettings().m_addFinalNewLine);
    QVERIFY(!document->configuredStorageSettings().m_addFinalNewLine);

    // A temporary change, like the one CppEditorDocument::saveImpl() makes for formatting.
    {
        StorageSettingsData saved = document->configuredStorageSettings();
        const auto restore = qScopeGuard([document, saved] {
            document->setStorageSettings(saved);
        });
        StorageSettingsData changed = saved;
        changed.m_cleanIndentation = false;
        document->setStorageSettings(changed);
    }

    // The .editorconfig setting of the old location does not stick to the document.
    document->setLineTerminationMode(TextFileFormat::LFLineTerminator);
    const FilePath moved = target.pathAppended("a.txt");
    QVERIFY(Core::DocumentManager::saveDocument(document, moved));
    QVERIFY(!document->storageSettings().m_addFinalNewLine);
    const Result<QByteArray> contents = moved.fileContents();
    QVERIFY(contents);
    QCOMPARE(*contents, QByteArray("x"));
}

void EditorConfigTest::testSaveUntitled()
{
    TemporaryDirectory tempDir("qtc-editorconfig-test");
    QVERIFY(tempDir.isValid());
    QVERIFY(tempDir.path()
                .pathAppended(".editorconfig")
                .writeFileContents("root = true\n"
                                   "[*]\n"
                                   "indent_style = tab\n"
                                   "insert_final_newline = true\n"));

    QString title = "untitled.txt";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, "x");
    QVERIFY(editor);
    const auto closeEditor = qScopeGuard([editor] {
        Core::EditorManager::closeEditors({editor}, false);
    });
    auto textEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(textEditor);
    TextDocument *document = textEditor->textDocument();
    QVERIFY(document->filePath().isEmpty());

    StorageSettingsData storageSettings;
    storageSettings.m_addFinalNewLine = false;
    document->setStorageSettings(storageSettings);
    document->setLineTerminationMode(TextFileFormat::LFLineTerminator);

    const FilePath saved = tempDir.path().pathAppended("untitled.txt");
    QVERIFY(Core::DocumentManager::saveDocument(document, saved));
    const Result<QByteArray> contents = saved.fileContents();
    QVERIFY(contents);
    QCOMPARE(*contents, QByteArray("x\n"));
    QCOMPARE(document->tabSettings().m_tabPolicy, TabSettingsData::TabsOnlyTabPolicy);
}

// Like the git commit message editor, which uses the commit encoding of the repository.
class EncodingChoosingWidget final : public TextEditorWidget
{
public:
    void aboutToOpen(const FilePath &, const FilePath &) final
    {
        textDocument()->setEncoding(TextEncoding(TextEncoding::Latin1));
    }
};

void EditorConfigTest::testEditorEncodingWins()
{
    TemporaryDirectory tempDir("qtc-editorconfig-test");
    QVERIFY(tempDir.isValid());
    QVERIFY(tempDir.path()
                .pathAppended(".editorconfig")
                .writeFileContents("root = true\n[*]\ncharset = utf-8\n"));
    const FilePath file = tempDir.path().pathAppended("COMMIT_EDITMSG");
    QVERIFY(file.writeFileContents("\xe4\n"));

    EncodingChoosingWidget widget;
    widget.setTextDocument(TextDocumentPtr(new TextDocument));
    TextDocument *document = widget.textDocument();
    QVERIFY(document->open(file, file));

    QCOMPARE(document->encoding().name(), TextEncoding(TextEncoding::Latin1).name());
    QCOMPARE(document->plainText(), QString(QChar(0xe4)) + '\n');
    QVERIFY(!document->editorConfig().encoding);
}

QObject *createEditorConfigTest()
{
    return new EditorConfigTest;
}

} // namespace TextEditor::Internal

#include "editorconfig_test.moc"
