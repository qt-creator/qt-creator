// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfix_test.h"

#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cppmodelmanager.h"
#include "../cppsourceprocessertesthelper.h"
#include "../cpptoolssettings.h"
#include "cppquickfix.h"
#include "cppquickfixassistant.h"

#include <projectexplorer/kitmanager.h>
#include <texteditor/textdocument.h>

#include <QByteArrayList>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QtTest>

/*!
    Tests for quick-fixes.
 */
using namespace Core;
using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

using CppEditor::Tests::TemporaryDir;
using CppEditor::Tests::Internal::TestIncludePaths;

namespace CppEditor {
namespace Internal {
namespace Tests {

QList<TestDocumentPtr> singleDocument(
    const QByteArray &original, const QByteArray &expected, const QByteArray fileName)
{
    return {CppTestDocument::create(fileName, original, expected)};
}

BaseQuickFixTestCase::BaseQuickFixTestCase(const QList<TestDocumentPtr> &testDocuments,
                                           const ProjectExplorer::HeaderPaths &headerPaths,
                                           const QByteArray &clangFormatSettings)
    : m_testDocuments(testDocuments)
    , m_cppCodeStylePreferences(0)
    , m_restoreHeaderPaths(false)
{
    QVERIFY(succeededSoFar());
    m_succeededSoFar = false;

    // Check if there is exactly one cursor marker
    unsigned cursorMarkersCount = 0;
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments)) {
        if (document->hasCursorMarker())
            ++cursorMarkersCount;
    }
    QVERIFY2(cursorMarkersCount == 1, "Exactly one cursor marker is allowed.");

    // Write documents to disk
    m_temporaryDirectory.reset(new TemporaryDir);
    QVERIFY(m_temporaryDirectory->isValid());
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments)) {
        if (QFileInfo(document->m_fileName).isRelative())
            document->setBaseDirectory(m_temporaryDirectory->path());
        document->writeToDisk();
    }

    // Create .clang-format file
    if (!clangFormatSettings.isEmpty())
        m_temporaryDirectory->createFile(".clang-format", clangFormatSettings);

    // Set appropriate include paths
    if (!headerPaths.isEmpty()) {
        m_restoreHeaderPaths = true;
        m_headerPathsToRestore = CppModelManager::headerPaths();
        CppModelManager::setHeaderPaths(headerPaths);
    }

    // Update Code Model
    QSet<FilePath> filePaths;
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments))
        filePaths << document->filePath();
    QVERIFY(parseFiles(filePaths));

    // Open Files
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments)) {
        QVERIFY(openCppEditor(document->filePath(), &document->m_editor,
                              &document->m_editorWidget));
        closeEditorAtEndOfTestCase(document->m_editor);

        // Set cursor position
        if (document->hasCursorMarker()) {
            if (document->hasAnchorMarker()) {
                document->m_editor->setCursorPosition(document->m_anchorPosition);
                document->m_editor->select(document->m_cursorPosition);
            } else {
                document->m_editor->setCursorPosition(document->m_cursorPosition);
            }
        } else {
            document->m_editor->setCursorPosition(0);
        }

        // Rehighlight
        QVERIFY(waitForRehighlightedSemanticDocument(document->m_editorWidget));
    }

    // Enforce the default cpp code style, so we are independent of config file settings.
    // This is needed by e.g. the GenerateGetterSetter quick fix.
    m_cppCodeStylePreferences = CppToolsSettings::cppCodeStyle();
    QVERIFY(m_cppCodeStylePreferences);
    m_cppCodeStylePreferencesOriginalDelegateId = m_cppCodeStylePreferences->currentDelegateId();
    m_cppCodeStylePreferences->setCurrentDelegate("qt");

    // Find the document having the cursor marker
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments)) {
        if (document->hasCursorMarker()){
            m_documentWithMarker = document;
            break;
        }
    }

    QVERIFY(m_documentWithMarker);
    m_succeededSoFar = true;
}

BaseQuickFixTestCase::~BaseQuickFixTestCase()
{
    // Restore default cpp code style
    if (m_cppCodeStylePreferences)
        m_cppCodeStylePreferences->setCurrentDelegate(m_cppCodeStylePreferencesOriginalDelegateId);

    // Restore include paths
    if (m_restoreHeaderPaths)
        CppModelManager::setHeaderPaths(m_headerPathsToRestore);

    // Remove created files from file system
    for (const TestDocumentPtr &testDocument : std::as_const(m_testDocuments))
        QVERIFY(testDocument->filePath().removeFile());
}

QuickFixOfferedOperationsTest::QuickFixOfferedOperationsTest(
        const QList<TestDocumentPtr> &testDocuments,
        CppQuickFixFactory *factory,
        const ProjectExplorer::HeaderPaths &headerPaths,
        const QStringList &expectedOperations)
    : BaseQuickFixTestCase(testDocuments, headerPaths)
{
    // Get operations
    CppQuickFixInterface quickFixInterface(m_documentWithMarker->m_editorWidget, ExplicitlyInvoked);
    QuickFixOperations actualOperations;
    factory->match(quickFixInterface, actualOperations);

    // Convert to QStringList
    QStringList actualOperationsAsStringList;
    for (const QuickFixOperation::Ptr &operation : std::as_const(actualOperations))
        actualOperationsAsStringList << operation->description();

    QCOMPARE(actualOperationsAsStringList, expectedOperations);
}

/// Leading whitespace is not removed, so we can check if the indetation ranges
/// have been set correctly by the quick-fix.
static QString &removeTrailingWhitespace(QString &input)
{
    const QStringList lines = input.split(QLatin1Char('\n'));
    input.resize(0);
    for (int i = 0, total = lines.size(); i < total; ++i) {
        QString line = lines.at(i);
        while (line.length() > 0) {
            QChar lastChar = line[line.length() - 1];
            if (lastChar == QLatin1Char(' ') || lastChar == QLatin1Char('\t'))
                line.chop(1);
            else
                break;
        }
        input.append(line);

        const bool isLastLine = i == lines.size() - 1;
        if (!isLastLine)
            input.append(QLatin1Char('\n'));
    }
    return input;
}

QuickFixOperationTest::QuickFixOperationTest(const QList<TestDocumentPtr> &testDocuments,
                                             CppQuickFixFactory *factory,
                                             const ProjectExplorer::HeaderPaths &headerPaths,
                                             int operationIndex,
                                             const QByteArray &expectedFailMessage,
                                             const QByteArray &clangFormatSettings)
    : BaseQuickFixTestCase(testDocuments, headerPaths, clangFormatSettings)
{
    if (factory->clangdReplacement() && CppModelManager::isClangCodeModelActive())
        return;

    QVERIFY(succeededSoFar());

    // Perform operation if there is one
    CppQuickFixInterface quickFixInterface(m_documentWithMarker->m_editorWidget, ExplicitlyInvoked);
    QuickFixOperations operations;
    factory->match(quickFixInterface, operations);
    if (operations.isEmpty()) {
        QEXPECT_FAIL("QTCREATORBUG-25998", "FIXME", Abort);
        QVERIFY(testDocuments.first()->m_expectedSource.isEmpty());
        return;
    }

    QVERIFY(operationIndex < operations.size());
    const QuickFixOperation::Ptr operation = operations.at(operationIndex);
    operation->perform();

    // Compare all files
    for (const TestDocumentPtr &testDocument : std::as_const(m_testDocuments)) {
        // Check
        QString result = testDocument->m_editorWidget->document()->toPlainText();
        removeTrailingWhitespace(result);
        QEXPECT_FAIL("escape-raw-string", "FIXME", Continue);
        QEXPECT_FAIL("unescape-adjacent-literals", "FIXME", Continue);
        if (!expectedFailMessage.isEmpty())
            QEXPECT_FAIL("", expectedFailMessage.data(), Continue);
        else if (result != testDocument->m_expectedSource) {
            qDebug() << "---" << testDocument->m_expectedSource;
            qDebug() << "+++" << result;
        }
        QCOMPARE(result, testDocument->m_expectedSource);

        // Undo the change
        for (int i = 0; i < 100; ++i)
            testDocument->m_editorWidget->undo();
        result = testDocument->m_editorWidget->document()->toPlainText();
        QCOMPARE(result, testDocument->m_source);
    }
}

void QuickFixOperationTest::run(const QList<TestDocumentPtr> &testDocuments,
                                CppQuickFixFactory *factory,
                                const QString &headerPath,
                                int operationIndex)
{
    ProjectExplorer::HeaderPaths headerPaths;
    headerPaths.push_back(ProjectExplorer::HeaderPath::makeUser(headerPath));
    QuickFixOperationTest(testDocuments, factory, headerPaths, operationIndex);
}

CppQuickFixTestObject::~CppQuickFixTestObject() = default;

CppQuickFixTestObject::CppQuickFixTestObject(std::unique_ptr<CppQuickFixFactory> &&factory)
    : m_factory(std::move(factory)) {}

void CppQuickFixTestObject::initTestCase()
{
    QString testName = objectName();
    if (testName.isEmpty()) {
        const QStringList classNameComponents
            = QString::fromLatin1(metaObject()->className()).split("::", Qt::SkipEmptyParts);
        QVERIFY(!classNameComponents.isEmpty());
        testName = classNameComponents.last();
    }
    const QDir testDir(QLatin1String(":/cppeditor/testcases/") + testName);
    QVERIFY2(testDir.exists(), qPrintable(testDir.absolutePath()));
    const QStringList subDirs = testDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &subDir : subDirs) {
        TestData testData;
        QDirIterator dit(testDir.absoluteFilePath(subDir));
        while (dit.hasNext()) {
            const QFileInfo fi = dit.nextFileInfo();
            const auto readFile = [&]() -> Utils::expected<QByteArray, QString> {
                QFile f(fi.absoluteFilePath());
                if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
                    return Utils::make_unexpected(
                        Tr::tr("Cannot open \"%1\"").arg(f.fileName()));
                return f.readAll();
            };
            if (fi.fileName() == "description.txt") {
                const auto t = readFile();
                if (!t)
                    QVERIFY2(false, qPrintable(t.error()));
                testData.tag = t->trimmed();
                continue;
            }
            if (fi.fileName() == "opindex.txt") {
                const auto t = readFile();
                if (!t)
                    QVERIFY2(false, qPrintable(t.error()));
                bool ok;
                testData.opIndex = t->trimmed().toInt(&ok);
                QVERIFY2(ok && testData.opIndex >= 0, t->constData());
                continue;
            }
            if (fi.fileName() == "fail.txt") {
                const auto m = readFile();
                if (!m)
                    QVERIFY2(false, qPrintable(m.error()));
                testData.failMessage = m->trimmed();
                continue;
            }
            if (fi.fileName() == "properties.txt") {
                const auto p = readFile();
                if (!p)
                    QVERIFY2(false, qPrintable(p.error()));
                const QByteArrayList lines = p->trimmed().split('\n');
                for (QByteArray line : lines) {
                    line = line.trimmed();
                    if (line.isEmpty())
                        continue;
                    const int colonOffset = line.indexOf(':');
                    if (colonOffset == -1) {
                        testData.properties.insert(QString::fromUtf8(line), true);
                    } else {
                        testData.properties.insert(
                            QString::fromUtf8(line.left(colonOffset)),
                            QString::fromUtf8(line.mid(colonOffset + 1)));
                    }
                }
                continue;
            }
            if (fi.fileName().startsWith("original_")) {
                const auto o = readFile();
                if (!o)
                    QVERIFY2(false, qPrintable(o.error()));
                QVERIFY2(!o->isEmpty(), qPrintable(fi.absoluteFilePath()));
                testData.files[fi.fileName().mid(9)].first = *o;
                continue;
            }
            if (fi.fileName().startsWith("expected_")) {
                const auto e = readFile();
                if (!e)
                    QVERIFY2(false, qPrintable(e.error()));
                testData.files[fi.fileName().mid(9)].second = *e;
                continue;
            }
            QVERIFY2(false, qPrintable(fi.absoluteFilePath()));
        }
        if (testData.tag.isEmpty())
            testData.tag = subDir.toUtf8();
        QVERIFY(!testData.files.isEmpty());
        m_testData.push_back(std::move(testData));
    }
    QVERIFY(!m_testData.isEmpty());
}

void CppQuickFixTestObject::cleanupTestCase()
{
    m_testData.clear();
}

void CppQuickFixTestObject::test_data()
{
    QTest::addColumn<QByteArrayList>("fileNames");
    QTest::addColumn<QByteArrayList>("original");
    QTest::addColumn<QByteArrayList>("expected");
    QTest::addColumn<int>("opIndex");
    QTest::addColumn<QByteArray>("failMessage");
    QTest::addColumn<QVariantMap>("properties");

    for (const TestData &testData : std::as_const(m_testData)) {
        QByteArrayList fileNames;
        QByteArrayList original;
        QByteArrayList expected;
        for (auto it = testData.files.begin(); it != testData.files.end(); ++it) {
            fileNames << it.key().toUtf8();
            original << it.value().first;
            expected << it.value().second;
        }
        QTest::newRow(testData.tag.constData()) << fileNames << original << expected
                                                << testData.opIndex << testData.failMessage
                                                << testData.properties;
    }
}

void CppQuickFixTestObject::test()
{
    QFETCH(QByteArrayList, fileNames);
    QFETCH(QByteArrayList, original);
    QFETCH(QByteArrayList, expected);
    QFETCH(int, opIndex);
    QFETCH(QByteArray, failMessage);
    QFETCH(QVariantMap, properties);

    class PropertiesMgr
    {
    public:
        PropertiesMgr(CppQuickFixFactory &factory, const QVariantMap &props)
            : m_factory(factory), m_props(props)
        {
            for (auto it = props.begin(); it != props.end(); ++it)
                m_factory.setProperty(it.key().toUtf8().constData(), it.value());
        }
        ~PropertiesMgr()
        {
            for (auto it = m_props.begin(); it != m_props.end(); ++it)
                m_factory.setProperty(it.key().toUtf8().constData(), {});
        }

    private:
        CppQuickFixFactory &m_factory;
        const QVariantMap &m_props;
    } propsMgr(*m_factory, properties);

    QList<TestDocumentPtr> testDocuments;
    for (qsizetype i = 0; i < fileNames.size(); ++i)
        testDocuments << CppTestDocument::create(fileNames.at(i), original.at(i), expected.at(i));
    QuickFixOperationTest(testDocuments, m_factory.get(), {}, opIndex, failMessage);
}

} // namespace Tests
} // namespace Internal
} // namespace CppEditor

