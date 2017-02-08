/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "formeditorplugin.h"

#include "formeditorw.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/testdatadir.h>
#include <cpptools/builtineditordocumentprocessor.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolstestcase.h>
#include <cpptools/editordocumenthandle.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>
#include <utils/fileutils.h>

#include <QDesignerFormEditorInterface>
#include <QDesignerIntegrationInterface>
#include <QStringList>
#include <QtTest>

using namespace Core;
using namespace Core::Tests;
using namespace CppTools;
using namespace CPlusPlus;
using namespace Designer;
using namespace Designer::Internal;

namespace {

QTC_DECLARE_MYTESTDATADIR("../../../tests/designer/")

class DocumentContainsFunctionDefinition: protected SymbolVisitor
{
public:
    bool operator()(Scope *scope, const QString function)
    {
        if (!scope)
            return false;

        m_referenceFunction = function;
        m_result = false;

        accept(scope);
        return m_result;
    }

protected:
    bool preVisit(Symbol *) { return !m_result; }

    bool visit(Function *symbol)
    {
        const QString function = m_overview.prettyName(symbol->name());
        if (function == m_referenceFunction)
            m_result = true;
        return false;
    }

private:
    bool m_result;
    QString m_referenceFunction;
    Overview m_overview;
};

class DocumentContainsDeclaration: protected SymbolVisitor
{
public:
    bool operator()(Scope *scope, const QString &function)
    {
        if (!scope)
            return false;

        m_referenceFunction = function;
        m_result = false;

        accept(scope);
        return m_result;
    }

protected:
    bool preVisit(Symbol *) { return !m_result; }

    void postVisit(Symbol *symbol)
    {
        if (symbol->isClass())
            m_currentClass.clear();
    }

    bool visit(Class *symbol)
    {
        m_currentClass = m_overview.prettyName(symbol->name());
        return true;
    }

    bool visit(Declaration *symbol)
    {
        QString declaration = m_overview.prettyName(symbol->name());
        if (!m_currentClass.isEmpty())
            declaration = m_currentClass + QLatin1String("::") + declaration;
        if (m_referenceFunction == declaration)
            m_result = true;
        return false;
    }

private:
    bool m_result;
    QString m_referenceFunction;
    QString m_currentClass;
    Overview m_overview;
};

bool documentContainsFunctionDefinition(const Document::Ptr &document, const QString &function)
{
    return DocumentContainsFunctionDefinition()(document->globalNamespace(), function);
}

bool documentContainsMemberFunctionDeclaration(const Document::Ptr &document,
                                               const QString &declaration)
{
    return DocumentContainsDeclaration()(document->globalNamespace(), declaration);
}

class GoToSlotTestCase : public CppTools::Tests::TestCase
{
public:
    GoToSlotTestCase(const QStringList &files)
    {
        QVERIFY(succeededSoFar());
        QCOMPARE(files.size(), 3);

        QList<TextEditor::BaseTextEditor *> editors;
        foreach (const QString &file, files) {
            IEditor *editor = EditorManager::openEditor(file);
            TextEditor::BaseTextEditor *e = qobject_cast<TextEditor::BaseTextEditor *>(editor);
            QVERIFY(e);
            closeEditorAtEndOfTestCase(editor);
            editors << e;
        }

        const QString cppFile = files.at(0);
        const QString hFile = files.at(1);

        QCOMPARE(DocumentModel::openedDocuments().size(), files.size());
        waitForFilesInGlobalSnapshot({ cppFile, hFile });

        // Execute "Go To Slot"
        QDesignerIntegrationInterface *integration = FormEditorW::designerEditor()->integration();
        QVERIFY(integration);
        integration->emitNavigateToSlot(QLatin1String("pushButton"), QLatin1String("clicked()"),
                                        QStringList());

        QCOMPARE(EditorManager::currentDocument()->filePath().toString(), cppFile);
        QVERIFY(EditorManager::currentDocument()->isModified());

        // Wait for updated documents
        foreach (TextEditor::BaseTextEditor *editor, editors) {
            const QString filePath = editor->document()->filePath().toString();
            if (auto parser = BuiltinEditorDocumentParser::get(filePath)) {
                forever {
                    if (Document::Ptr document = parser->document()) {
                        if (document->editorRevision() == 2)
                            break;
                    }
                    QApplication::processEvents();
                }
            }
        }

        // Compare
        const auto cppDocumentParser = BuiltinEditorDocumentParser::get(cppFile);
        QVERIFY(cppDocumentParser);
        const Document::Ptr cppDocument = cppDocumentParser->document();
        QVERIFY(checkDiagsnosticMessages(cppDocument));

        const auto hDocumentParser = BuiltinEditorDocumentParser::get(hFile);
        QVERIFY(hDocumentParser);
        const Document::Ptr hDocument = hDocumentParser->document();
        QVERIFY(checkDiagsnosticMessages(hDocument));

        QVERIFY(documentContainsFunctionDefinition(cppDocument,
            QLatin1String("Form::on_pushButton_clicked")));
        QVERIFY(documentContainsMemberFunctionDeclaration(hDocument,
            QLatin1String("Form::on_pushButton_clicked")));
    }

    static bool checkDiagsnosticMessages(const Document::Ptr &document)
    {
        if (!document)
            return false;

        // Since no project is opened and the ui_*.h is not generated,
        // the following diagnostic messages will be ignored.
        const QStringList ignoreList = QStringList({ "ui_form.h: No such file or directory",
                                                     "QWidget: No such file or directory" });
        QList<Document::DiagnosticMessage> cleanedDiagnosticMessages;
        foreach (const Document::DiagnosticMessage &message, document->diagnosticMessages()) {
            if (!ignoreList.contains(message.text()))
                cleanedDiagnosticMessages << message;
        }

        return cleanedDiagnosticMessages.isEmpty();
    }
};

} // anonymous namespace

namespace Designer {
namespace Internal {

/// Check: Executes "Go To Slot..." on a QPushButton in a *.ui file and checks if the respective
/// header and source files are correctly updated.
void FormEditorPlugin::test_gotoslot()
{
    QFETCH(QStringList, files);
    (GoToSlotTestCase(files));
}

void FormEditorPlugin::test_gotoslot_data()
{
    typedef QLatin1String _;
    QTest::addColumn<QStringList>("files");

    MyTestDataDir testDataDirWithoutProject(_("gotoslot_withoutProject"));
    QTest::newRow("withoutProject")
        << QStringList({ testDataDirWithoutProject.file(_("form.cpp")),
                         testDataDirWithoutProject.file(_("form.h")),
                         testDataDirWithoutProject.file(_("form.ui")) });

    // Finding the right class for inserting definitions/declarations is based on
    // finding a class with a member whose type is the class from the "ui_xxx.h" header.
    // In the following test data the header files contain an extra class referencing
    // the same class name.

    MyTestDataDir testDataDir;

    testDataDir = MyTestDataDir(_("gotoslot_insertIntoCorrectClass_pointer"));
    QTest::newRow("insertIntoCorrectClass_pointer")
        << QStringList({ testDataDir.file(_("form.cpp")), testDataDir.file(_("form.h")),
                         testDataDirWithoutProject.file(_("form.ui")) }); // reuse

    testDataDir = MyTestDataDir(_("gotoslot_insertIntoCorrectClass_non-pointer"));
    QTest::newRow("insertIntoCorrectClass_non-pointer")
        << QStringList({ testDataDir.file(_("form.cpp")), testDataDir.file(_("form.h")),
                         testDataDirWithoutProject.file(_("form.ui")) }); // reuse

    testDataDir = MyTestDataDir(_("gotoslot_insertIntoCorrectClass_pointer_ns_using"));
    QTest::newRow("insertIntoCorrectClass_pointer_ns_using")
        << QStringList({ testDataDir.file(_("form.cpp")), testDataDir.file(_("form.h")),
                         testDataDir.file(_("form.ui")) });
}

} // namespace Internal
} // namespace Designer
