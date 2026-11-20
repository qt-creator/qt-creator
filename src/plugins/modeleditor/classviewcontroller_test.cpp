// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "classviewcontroller.h"

#include <cppeditor/cpptoolstestcase.h>

#include <QTest>

using namespace Utils;

namespace ModelEditor::Internal {

class ClassViewControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void testClassesInFilesTheIndexerDoesNotDeliverAreFound();
};

void ClassViewControllerTest::testClassesInFilesTheIndexerDoesNotDeliverAreFound()
{
    ProjectExplorer::Kit * const kit = CppEditor::Tests::kitWithQt();
    if (!kit)
        QSKIP("This test needs a kit with Qt to build the project");

    CppEditor::Tests::TemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const FilePath header = tmpDir.createFile("foo.h", "class Foo {};\n");
    const FilePath source = tmpDir.createFile("foo.cpp",
                                              "#include \"foo.h\"\nclass FooPrivate {};\n");
    QVERIFY(!header.isEmpty());
    QVERIFY(!source.isEmpty());
    QVERIFY(!tmpDir.createFile("main.cpp", "int main() { return 0; }\n").isEmpty());
    const FilePath projectFile = tmpDir.createFile(
        "p.pro", "TEMPLATE = app\nSOURCES = foo.cpp main.cpp\nHEADERS = foo.h\n");
    QVERIFY(!projectFile.isEmpty());

    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(projectFile, kit));
    QVERIFY(CppEditor::Tests::TestCase::waitForFilesInGlobalSnapshot({header, source})
                .size() == 2);

    // The index still has the files, the snapshot no longer has them.
    CppEditor::Tests::removeFromSnapshot({header, source});

    // Reading on request starts once the project is fully indexed.
    ClassViewController controller;
    QTRY_COMPARE(controller.findClassDeclarations(header),
                 QSet<QString>({"Foo", "FooPrivate"}));
}

QObject *createClassViewControllerTest()
{
    return new ClassViewControllerTest;
}

} // namespace ModelEditor::Internal

#include "classviewcontroller_test.moc"
