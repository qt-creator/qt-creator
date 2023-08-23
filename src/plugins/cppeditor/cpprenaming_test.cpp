// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpprenaming_test.h"

#include "cppeditorwidget.h"
#include "cppmodelmanager.h"
#include "cppquickfix_test.h"

#include <texteditor/texteditor.h>

#include <QEventLoop>
#include <QTimer>
#include <QtTest>

namespace CppEditor::Internal::Tests {

class RenamingTestRunner : public BaseQuickFixTestCase
{
public:
    RenamingTestRunner(const QList<TestDocumentPtr> &testDocuments, const QString &replacement)
        : BaseQuickFixTestCase(testDocuments, {})
    {
        QVERIFY(succeededSoFar());
        const TestDocumentPtr &doc = m_documentWithMarker;
        const CursorInEditor cursorInEditor(doc->m_editor->textCursor(), doc->filePath(),
                                            doc->m_editorWidget, doc->m_editor->textDocument());

        QEventLoop loop;
        CppModelManager::globalRename(cursorInEditor, replacement, [&loop]{ loop.quit(); });
        QTimer::singleShot(10000, &loop, [&loop] { loop.exit(1); });
        QVERIFY(loop.exec() == 0);

        // Compare all files
        for (const TestDocumentPtr &testDocument : std::as_const(m_testDocuments)) {
            QString result = testDocument->m_editorWidget->document()->toPlainText();
            if (result != testDocument->m_expectedSource) {
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
};

void GlobalRenamingTest::test_data()
{
    QTest::addColumn<QByteArrayList>("headers");
    QTest::addColumn<QByteArrayList>("sources");
    QTest::addColumn<QString>("replacement");

    const char testClassHeader[] = R"cpp(
/**
 * \brief MyClass
 */
class MyClass {
  /** \brief MyClass::MyClass */
  MyClass() {}
  ~MyClass();
  /** \brief MyClass::run */
  void run();
};
)cpp";

    const char testClassSource[] = R"cpp(
#include "file.h"
/** \brief MyClass::~MyClass */
MyClass::~MyClass() {}

void MyClass::run() {}
)cpp";

    QByteArray origHeaderClassName(testClassHeader);
    const int classOffset = origHeaderClassName.indexOf("class MyClass");
    QVERIFY(classOffset != -1);
    origHeaderClassName.insert(classOffset + 6, '@');
    const QByteArray newHeaderClassName = R"cpp(
/**
 * \brief MyNewClass
 */
class MyNewClass {
  /** \brief MyNewClass::MyNewClass */
  MyNewClass() {}
  ~MyNewClass();
  /** \brief MyNewClass::run */
  void run();
};
)cpp";
    const QByteArray newSourceClassName = R"cpp(
#include "file.h"
/** \brief MyNewClass::~MyNewClass */
MyNewClass::~MyNewClass() {}

void MyNewClass::run() {}
)cpp";
    QTest::newRow("class name") << QByteArrayList{origHeaderClassName, newHeaderClassName}
                                << QByteArrayList{testClassSource, newSourceClassName}
                                << QString("MyNewClass");

    QByteArray origSourceMethodName(testClassSource);
    const int methodOffset = origSourceMethodName.indexOf("::run()");
    QVERIFY(methodOffset != -1);
    origSourceMethodName.insert(methodOffset + 2, '@');
    const QByteArray newHeaderMethodName = R"cpp(
/**
 * \brief MyClass
 */
class MyClass {
  /** \brief MyClass::MyClass */
  MyClass() {}
  ~MyClass();
  /** \brief MyClass::runAgain */
  void runAgain();
};
)cpp";
    const QByteArray newSourceMethodName = R"cpp(
#include "file.h"
/** \brief MyClass::~MyClass */
MyClass::~MyClass() {}

void MyClass::runAgain() {}
)cpp";
    QTest::newRow("method name") << QByteArrayList{testClassHeader, newHeaderMethodName}
                                 << QByteArrayList{origSourceMethodName, newSourceMethodName}
                                 << QString("runAgain");
}

void GlobalRenamingTest::test()
{
    QFETCH(QByteArrayList, headers);
    QFETCH(QByteArrayList, sources);
    QFETCH(QString, replacement);

    QList<TestDocumentPtr> testDocuments(
        {CppTestDocument::create("file.h", headers.at(0), headers.at(1)),
         CppTestDocument::create("file.cpp", sources.at(0), sources.at(1))});
    RenamingTestRunner testRunner(testDocuments, replacement);
}

} // namespace CppEditor::Internal::Tests
