/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "testcode.h"

#ifndef QTTEST_PLUGIN_LEAN
# include "qsystemtest.h"
#endif

#include <coreplugin/ifile.h>
#include <coreplugin/icore.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <extensionsystem/pluginmanager.h>
#include <texteditor/basetexteditor.h>

#include <coreplugin/icontext.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/parser/qmljsast_p.h>


#include <AST.h>
#include <ASTVisitor.h>
#include <Literals.h>
#include <cplusplus/LookupContext.h>
#include <Symbols.h>
#include <SymbolVisitor.h>

#include <TranslationUnit.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/ModelManagerInterface.h>

#include <QFile>
#include <QStringList>
#include <QDir>
#include <QTextStream>
#include <QIODevice>
#include <QRegExp>
#include <QDebug>

class SystemTestCodeSync : protected QmlJS::AST::Visitor
{
public:
    SystemTestCodeSync(TestCode *testCode) :
        m_foundTestCase(false),
        m_testCode(testCode)
    {
    }

    void operator()(QmlJS::Document::Ptr doc)
    {
        if (doc && doc->ast())
            doc->ast()->accept(this);
    }

private:
    bool visit(QmlJS::AST::IdentifierExpression *identifier)
    {
        const QStringRef &name = identifier->name;
        m_foundTestCase = (name == QLatin1String("testcase"));
        if (!m_foundTestCase && (name == QLatin1String("prompt") || name == QLatin1String("manualTest")))
            m_testCode->setManualTest(identifier->identifierToken.offset);
        return true;
    }

    bool visit(QmlJS::AST::ObjectLiteral *objectLiteral)
    {
        if (m_foundTestCase) {
            QmlJS::AST::PropertyNameAndValueList *properties = objectLiteral->properties;
            visitProperties(properties);
            m_foundTestCase = false;
            return true;
        } else {
            return false;
        }
    }

    void visitProperties(QmlJS::AST::PropertyNameAndValueList *properties)
    {
        if (!properties)
            return;

        m_testCode->clearTestFunctions();
        while (properties) {
            if (properties->name->kind == QmlJS::AST::Node::Kind_IdentifierPropertyName) {
                QmlJS::AST::IdentifierPropertyName *name =
                    static_cast<QmlJS::AST::IdentifierPropertyName*>(properties->name);
                const QString &nameString = name->id.toString();
                if (properties->value->kind == QmlJS::AST::Node::Kind_FunctionExpression) {
                    int startLine = name->propertyNameToken.startLine;
                    int start = name->propertyNameToken.offset;
                    int end =
                        static_cast<QmlJS::AST::FunctionExpression*>(properties->value)->rbraceToken.offset;
                    m_testCode->processFunction(nameString, startLine, start, end);
                }
            }
            properties = properties->next;
        }
    }
    bool m_foundTestCase;
    TestCode *m_testCode;
};

class UnitTestCodeSync : protected CPlusPlus::SymbolVisitor
{
    TestCode *m_testCode;
    CPlusPlus::Document::Ptr m_doc;
    CPlusPlus::Snapshot m_snapshot;
    QSet<QByteArray> m_virtualMethods;
    QString m_path;

public:
    UnitTestCodeSync(TestCode *tc, CPlusPlus::Document::Ptr doc, const CPlusPlus::Snapshot &snapshot) :
        m_testCode(tc),
        m_doc(doc),
        m_snapshot(snapshot),
        m_path(QFileInfo(doc->fileName()).absolutePath())
    {
        QSet<CPlusPlus::Namespace *> processed;
        process(doc, &processed);
    }

    const QSet<QByteArray> &testFunctions() const
    {
        return m_virtualMethods;
    }

protected:
    void process(CPlusPlus::Document::Ptr doc, QSet<CPlusPlus::Namespace *> *processed)
    {
        if (!doc) {
            return;
        } else if (!processed->contains(doc->globalNamespace())) {
            processed->insert(doc->globalNamespace());

             foreach (const CPlusPlus::Document::Include &i, doc->includes()) {
                 if (QFileInfo(i.fileName()).absolutePath() == m_path)
                     process(m_snapshot.document(i.fileName()), processed);
             }

            accept(doc->globalNamespace());
        }
    }

    virtual bool visit(CPlusPlus::Function *symbol)
    {
        if (symbol->name()) {
            const CPlusPlus::QualifiedNameId *qn = symbol->name()->asQualifiedNameId();
            if (qn && qn->base() && qn->base()->identifier() && qn->name() && qn->name()->identifier()) {
                QString name = QString::fromLatin1("%1::%2").arg(qn->base()->identifier()->chars())
                    .arg(qn->name()->identifier()->chars());
                if (m_knownTestFunctions.contains(name))
                    m_testCode->processFunction(QString(symbol->name()->identifier()->chars()),
                        symbol->line(), symbol->startOffset(), symbol->endOffset());
            }
        }
        return true;
    }

    virtual bool visit(CPlusPlus::Class *symbol)
    {
        if (symbol->name() && symbol->name()->identifier()) {
            QString className = QString(symbol->name()->identifier()->chars());
            if (className != m_testCode->m_testCase) {
                if (!m_testCode->m_testCase.isEmpty())
                    return true;
            }
            for (CPlusPlus::Scope::iterator it = symbol->firstMember(); it != symbol->lastMember(); ++it) {
                CPlusPlus::Symbol *member = *it;
                CPlusPlus::Function *fun = member->type()->asFunctionType();
                if (fun && fun->isSlot() && member && member->name() && member->name()->identifier()) {
                    m_knownTestFunctions.append(className + QLatin1String("::")
                        + QString(member->name()->identifier()->chars()));
                    m_testCode->processFunction(QString(member->name()->identifier()->chars()),
                        fun->line(), fun->startOffset(), fun->endOffset());
                }
            }
        }
        return true;
    }

    QStringList m_knownTestFunctions;
};

TestFunctionInfo::TestFunctionInfo() : QObject()
{
    reset();
}

TestFunctionInfo::~TestFunctionInfo()
{
}

TestFunctionInfo::TestFunctionInfo(TestFunctionInfo &other) : QObject()
{
    m_functionName = other.m_functionName;
    m_isManualTest = other.m_isManualTest;
    m_testStart = other.m_testStart;
    m_testStartLine = other.m_testStartLine;
    m_testEnd = other.m_testEnd;
    m_declStart = other.m_declStart;
    m_declStartLine = other.m_declStartLine;
    m_declEnd = other.m_declEnd;
    m_testGroups = other.m_testGroups;
}

void TestFunctionInfo::reset()
{
    m_functionName.clear();
    m_isManualTest = false;
    m_testStart = m_testStartLine = m_testEnd = -1;
    m_declStart = m_declStartLine = m_declEnd = -1;
    m_testGroups.clear();
}

TestFunctionInfo& TestFunctionInfo::operator=(const TestFunctionInfo &other)
{
    m_functionName = other.m_functionName;
    m_isManualTest = other.m_isManualTest;
    m_testStart = other.m_testStart;
    m_testStartLine = other.m_testStartLine;
    m_testEnd = other.m_testEnd;
    m_declStart = other.m_declStart;
    m_declStartLine = other.m_declStartLine;
    m_declEnd = other.m_declEnd;
    m_testGroups = other.m_testGroups;
    return *this;
}

bool TestFunctionInfo::validFunctionName(const QString &funcName)
{
    return (!funcName.isEmpty()
        && funcName != QLatin1String("init")
        && funcName != QLatin1String("initTestCase")
        && funcName != QLatin1String("cleanup")
        && funcName != QLatin1String("cleanupTestCase")
        && !funcName.endsWith(QLatin1String("_data")));
}

TestCode::TestCode(const QString &basePath, const QString &externalPath, const QString &fileName) :
    m_basePath(basePath),
    m_externalPath(externalPath),
    m_fileName(QDir::toNativeSeparators(fileName)),
    m_testType(TypeUnknownTest),
    m_hasChanged(false),
    m_codeEditor(0),
    m_fileInfo(new QFileInfo(m_fileName)),
    m_initialized(false),
    m_errored(false)
{
    QString baseName = baseFileName();
    if (baseName.endsWith(QLatin1String(".qtt")))
        m_testType = TypeSystemTest;
    else if (baseName.startsWith(QLatin1String("prf_")))
        m_testType = TypePerformanceTest;
    else if (baseName.startsWith(QLatin1String("int_")))
        m_testType = TypeIntegrationTest;
    else if (baseName.endsWith(QLatin1String(".cpp")))
        m_testType = TypeUnitTest;

    connect(&m_parseTimer, SIGNAL(timeout()), this, SLOT(parseDocument()), Qt::DirectConnection);
}

TestCode::~TestCode()
{
    delete m_fileInfo;
}

bool TestCode::parseComments(const QString &contents)
{
    static QRegExp componentRegEx(QLatin1String("//TESTED_COMPONENT=(.*)"));
    static QRegExp fileRegEx(QLatin1String("//TESTED_FILE=(.*)"));
    static QRegExp classRegEx(QLatin1String("//TESTED_CLASS=(.*)"));
    static QRegExp groupsRegEx(QLatin1String("\\\\groups\\s+(.*)"));

    m_testedComponent.clear();
    m_testedFile.clear();
    m_testedClass.clear();
    int offset = 0;

    QStringList fileContents = contents.split(QLatin1Char('\n'));
    foreach (const QString &line, fileContents) {
        if (m_testedComponent.isEmpty() && line.contains(componentRegEx)) {
            m_testedComponent = componentRegEx.cap(1);
        } else if (m_testedFile.isEmpty() && line.contains(fileRegEx)) {
            m_testedFile = fileRegEx.cap(1);
        } else if (m_testedClass.isEmpty() && line.contains(classRegEx)) {
            m_testedClass = classRegEx.cap(1);
        } else if (line.contains(groupsRegEx)) {
            TestFunctionInfo *tfi = findFunction(offset, true);
            if (tfi)
                tfi->setTestGroups(groupsRegEx.cap(1));
        }
        offset += line.length() + 1;
    }

    return true;
}

Core::IEditor *TestCode::editor()
{
    return m_codeEditor;
}

bool TestCode::openTestInEditor(const QString &testFunction)
{
    bool ok = true;
    Core::ICore *core = Core::ICore::instance();
    Core::EditorManager *em = core->editorManager();
    Core::IEditor *edit = em->openEditor(actualFileName());

    if (m_codeEditor && m_codeEditor != edit) {
        m_codeEditor = 0;
    }

    if (!m_codeEditor) {
        if (!edit)
            return false;

        m_codeEditor = qobject_cast<TextEditor::BaseTextEditor*>(edit);
        if (m_codeEditor) {
            if (m_fileName.endsWith(QLatin1String(".qtt"))) {
                connect(edit, SIGNAL(contextHelpIdRequested(TextEditor::ITextEditor*,int)),
                    this, SLOT(onContextHelpIdRequested(TextEditor::ITextEditor*,int)));
                m_codeEditor->setContextHelpId(QLatin1String("QtUiTest Manual"));
            }
        }

        m_hasChanged = false;
    }

    if (!testFunction.isEmpty()) {
        TestFunctionInfo *functionInfo = findFunction(testFunction);
        if (functionInfo) {
            if (m_testType == TypeSystemTest)
                m_codeEditor->setCursorPosition(functionInfo->testStart());
            else
                gotoLine(functionInfo->testStartLine());
        }
    }

    if (m_errored) {
        // Only System Tests can be errored right now...
        QmlJS::DiagnosticMessage msg = m_qmlJSDoc->diagnosticMessages().first();
        m_codeEditor->setCursorPosition(msg.loc.offset);
    }

    return ok;
}

/*!
    Returns TRUE if the specified \a funcName is a function of the class.
    funcName must be a normalized name, e.g. "myFunc(type var)"
*/

bool TestCode::testFunctionExists(QString funcName)
{
    for (int i = 0; i < m_testFunctions.count(); ++i) {
        TestFunctionInfo *inf = m_testFunctions.at(i);
        if (inf && inf->functionName() == funcName)
            return true;
    }
    return false;
}

/*!
    Returns the name of the Component that is tested.
*/

QString TestCode::testedComponent() const
{
    if (m_testedComponent.isEmpty())
        return QLatin1String("other");
    return m_testedComponent;
}

/*!
    Returns the (header) filename that contains the declaration of the class_under_test.
*/
QString TestCode::testedFile() const
{
    return m_testedFile;
}

/*!
    Returns the Class name of the class_under_test.
*/
QString TestCode::testedClass() const
{
    return m_testedClass;
}

/*!
    Returns the testcase name as found in the opened header file or the headerfile that
    is associated with the opened cpp file.
*/
QString TestCode::testCase() const
{
    return m_testCase;
}

void TestCode::clearTestFunctions()
{
    m_testFunctions.clear();
}

/*!
    Returns the number of testfunctions found in the testcase.
*/
uint TestCode::testFunctionCount()
{
    return m_testFunctions.count();
}

/*!
    Returns a pointer to the TestFunctionInfo instance that is associated to the
    testfunction specified by \a index or null if the function doesn't exist. If the testfunction
    is a manual test \a manualTest will return true;
*/
TestFunctionInfo* TestCode::testFunction(int index)
{
    if (index >= 0 && index < m_testFunctions.count())
        return m_testFunctions.at(index);
    return 0;
}

/*!
    Returns \a info for the test function that starts on or after \a offset.
    If \a next is specified it will return the Next function.
*/
TestFunctionInfo *TestCode::findFunction(int offset, bool next)
{
    if (offset < 0)
        return 0;
    TestFunctionInfo *found = 0;

    for (int i = 0; i < m_testFunctions.count(); ++i) {
        TestFunctionInfo *inf = m_testFunctions.at(i);
        if (inf) {
            if (next) {
                if ((inf->testEnd() > offset) && (inf->testStart() > offset)) {
                    if (!found || (found->testStart() > inf->testStart()))
                        found = inf;
                }
            } else {
                if (inf->testEnd() >= offset && inf->testStart() <= offset && inf->testStart() > 0)
                    return inf;
            }
        }
    }
    return found;
}

/*!
    Returns info for the test function named \a funcName.
*/
TestFunctionInfo *TestCode::findFunction(const QString &funcName)
{
    for (int i = 0; i < m_testFunctions.count(); ++i) {
        TestFunctionInfo *inf = m_testFunctions.at(i);
        if (inf && inf->functionName() == funcName)
            return inf;
    }
    return 0;
}

/*!
    Returns the test type.
*/
TestCode::TestType TestCode::testType() const
{
    return m_testType;
}

QString TestCode::testTypeString() const
{
    QString ret;
    switch (m_testType) {
    case TypeUnitTest:
        ret = QLatin1String("Unit");
        break;
    case TypeIntegrationTest:
        ret = QLatin1String("Integration");
        break;
    case TypePerformanceTest:
        ret = QLatin1String("Performance");
        break;
    case TypeSystemTest:
        ret = QLatin1String("System");
        break;
    default:
        ret = QLatin1String("Unknown");
        break;
    }
    return ret;
}

void TestCode::setDocument(QmlJS::Document::Ptr doc)
{
    if (m_qmlJSDoc)
        m_qmlJSDoc.clear();

    m_qmlJSDoc = doc;
    if (m_qmlJSDoc) {
        m_parseTimer.stop();
        m_parseTimer.setSingleShot(true);
        m_parseTimer.start(1000);
    }
}

void TestCode::setDocument(CPlusPlus::Document::Ptr doc)
{
    if (m_cppDoc)
        m_cppDoc.clear();

    m_cppDoc = doc;
    if (m_cppDoc) {
        m_parseTimer.stop();
        m_parseTimer.setSingleShot(true);
        m_parseTimer.start(1000);
    }
}

bool tfLt(const TestFunctionInfo *a, const TestFunctionInfo *b)
{
    return a->testStart() < b->testStart();
}

void TestCode::parseDocument()
{
    QString contents;
    if (m_codeEditor) {
        contents = m_codeEditor->contents();
    } else {
        if (m_testType == TypeSystemTest) {
            contents = m_qmlJSDoc->source();
        } else {
            QFile F(m_fileName);
            if (F.open(QIODevice::ReadOnly)) {
                QTextStream S(&F);
                contents = S.readAll();
                F.close();
            } else {
                return;
            }
        }
    }

    m_testCase.clear();

    if (m_testType == TypeSystemTest) {
        static QRegExp fileNameReg(QLatin1String(".*/(.*)/.*\\.qtt$"));
        if (fileNameReg.indexIn(QDir::fromNativeSeparators(m_qmlJSDoc->fileName())) >= 0)
            m_testCase = fileNameReg.cap(1);
        SystemTestCodeSync sync(this);
        sync(m_qmlJSDoc);
        m_errored = false;
        if (m_testFunctions.isEmpty() && !m_qmlJSDoc->diagnosticMessages().isEmpty())
            m_errored = true;
    } else {
        // Determine the testcase class name
        foreach (const CPlusPlus::Document::MacroUse &macro, m_cppDoc->macroUses()) {
            QString macroName(macro.macro().name());
            if (macro.isFunctionLike() && ((macroName == QLatin1String("QTEST_MAIN"))
                || (macroName == QLatin1String("QTEST_APPLESS_MAIN")) || (macroName == QLatin1String("QTEST_NOOP_MAIN")))) {
                int pos = macro.arguments()[0].position();
                int length = macro.arguments()[0].length();
                m_testCase = contents.mid(pos, length);
                break;
            }
        }

        clearTestFunctions();
        const CPlusPlus::Snapshot snapshot =
            CPlusPlus::CppModelManagerInterface::instance()->snapshot();
        UnitTestCodeSync sync(this, m_cppDoc, snapshot);
    }

    parseComments(contents);
    qStableSort(m_testFunctions.begin(), m_testFunctions.end(), tfLt);

    m_initialized = true;

    // TODO: only emit if something significant has changed
    emit testChanged(this);
}

TestFunctionInfo *TestCode::currentTestFunction()
{
    if (m_codeEditor) {
        int currentPos = m_codeEditor->position();
        for (int i = 0; i < m_testFunctions.count(); ++i) {
            TestFunctionInfo *inf = m_testFunctions.at(i);
            if (inf && (inf->testStart() <= currentPos) && (inf->testEnd() >= currentPos))
                return inf;
        }
    }
    return 0;
}

QString TestCode::projectFileName()
{
    // Figure out what the pro file is.
    QString srcPath = QDir::convertSeparators(m_fileInfo->absolutePath());
    QDir D(srcPath);
    QStringList files = D.entryList(QStringList(QLatin1String("*.pro")));
    if (files.count() > 1) {
        qDebug() << "CFAIL", "I am confused: Multiple .pro files (" + files.join(QString(QLatin1Char(',')))
            + ") found in '" + srcPath + "'.";
        return QString();
    }

    if (files.count() == 0)
        return QString();

    return QString(srcPath + QDir::separator() + files[0]);
}

QString TestCode::execFileName()
{
    QString proFile = projectFileName();
    if (proFile.isEmpty())
        return QString();

    QFile F(proFile);
    if (F.open(QFile::ReadOnly)) {
        QTextStream S(&F);
        while (!S.atEnd()) {
            QString line = S.readLine();
            if (line.contains(QLatin1String("TARGET"))) {
                line = line.mid(line.indexOf(QLatin1Char('=')) + 1);
                line = line.simplified();
                if (line.contains(QLatin1String("$$TARGET"))) {
                    QFileInfo inf(proFile);
                    line.replace(QLatin1String("$$TARGET"),inf.baseName());
                }
                return line;
            }
        }
    }
    // behave same as qmake, if no TARGET set then fallback to a
    //  name based on the .pro file name.
    if (m_testType == TypeUnitTest){
       QFileInfo fInfo(proFile);
       return fInfo.baseName();
    }

    return QString();
}


// ***************************************************************************

TestCollectionPrivate::TestCollectionPrivate() :
    m_currentEditedTest(0),
    m_qmlJSModelManager(0),
    m_cppModelManager(0)
{
    QTimer::singleShot(0, this, SLOT(initModelManager()));
}

TestCollectionPrivate::~TestCollectionPrivate()
{
    removeAll();
}

void TestCollectionPrivate::initModelManager()
{
    m_qmlJSModelManager =
        ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();
    if (m_qmlJSModelManager) {
        connect(m_qmlJSModelManager, SIGNAL(documentUpdated(QmlJS::Document::Ptr)),
            this, SLOT(onDocumentUpdated(QmlJS::Document::Ptr)));
    }

    m_cppModelManager = CPlusPlus::CppModelManagerInterface::instance();
    if (m_cppModelManager) {
        connect(m_cppModelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this, SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));
    }
}

void TestCollectionPrivate::onDocumentUpdated(QmlJS::Document::Ptr doc)
{
    TestCode *code;

    if (!m_qttDocumentMap.contains(doc->fileName())) {
        return;
    }

    if (!(code = m_qttDocumentMap[doc->fileName()])) {
        // TODO: New file, need to confirm it's a .qtt first
        code = findCode(doc->fileName(), m_currentScanRoot, m_currentExtraBase);
    }

    if (code)
        code->setDocument(doc);
}

void TestCollectionPrivate::onDocumentUpdated(CPlusPlus::Document::Ptr doc)
{
    const CPlusPlus::Snapshot snapshot = m_cppModelManager->snapshot();
    TestCode *code;

    if (!m_cppDocumentMap.contains(doc->fileName())) {
        // Don't care about this document unless it's a new test...
        // TODO: Ensure that new tests get scanned...
        return;
    }

    if (!(code = m_cppDocumentMap[doc->fileName()])) {
        if (isUnitTestCase(doc, snapshot)) {
            code = findCode(doc->fileName(), m_currentScanRoot, m_currentExtraBase);
        }
    }

    if (code)
        code->setDocument(doc);
}

bool TestCollectionPrivate::isUnitTestCase(const CPlusPlus::Document::Ptr &doc,
    const CPlusPlus::Snapshot &snapshot)
{
    QStringList visited;
    return isUnitTestCase(doc, snapshot, visited);
}

bool TestCollectionPrivate::isUnitTestCase(const CPlusPlus::Document::Ptr &doc,
    const CPlusPlus::Snapshot &snapshot, QStringList &visited)
{
    // If a source file #includes QtTest/QTest, assume it is a testcase
    foreach (const CPlusPlus::Document::Include &i, doc->includes()) {
        if (i.fileName().contains(QLatin1String("QtTest")) || i.fileName().contains(QLatin1String("QTest")))
            return true;

        if (visited.contains(i.fileName()))
            continue;

        visited << i.fileName();

        if (QFileInfo(i.fileName()).absolutePath() == QFileInfo(doc->fileName()).absolutePath()) {
            CPlusPlus::Document::Ptr incDoc = snapshot.document(i.fileName());
            if (incDoc && isUnitTestCase(incDoc, snapshot, visited))
                return true;
        }
    }
    return false;
}

void TestCollectionPrivate::removeAll()
{
    while (m_codeList.count() > 0)
        delete m_codeList.takeFirst();

    m_qttDocumentMap.clear();
    m_cppDocumentMap.clear();
}

void TestCollectionPrivate::removeCode(const QString &fileName)
{
    TestCode *tmp;
    for (int i = 0; i < m_codeList.count(); ++i) {
        tmp = m_codeList.at(i);
        if (tmp && tmp->actualFileName() == fileName) {
            emit testRemoved(tmp);
            delete m_codeList.takeAt(i);
            return;
        }
    }
}

void TestCollectionPrivate::removePath(const QString &srcPath)
{
    if (srcPath.isEmpty() || m_codeList.count() <= 0)
        return;

    TestCode *tmp;
    for (int i = m_codeList.count() - 1; i >= 0; --i) {
        tmp = m_codeList.at(i);
        if (tmp && tmp->actualFileName().startsWith(srcPath)) {
            emit testRemoved(tmp);
            delete m_codeList.takeAt(i);
        }
    }
}

void TestCollectionPrivate::addPath(const QString &srcPath)
{
    m_currentScanRoot = srcPath;
    m_currentExtraBase.clear();
    scanTests(srcPath);
    emit changed();
}

void TestCollectionPrivate::addExtraPath(const QString &srcPath)
{
    m_currentExtraBase = srcPath;
    scanTests(srcPath);
    emit changed();
}

void TestCollectionPrivate::scanTests(const QString &suitePath)
{
    if (suitePath.isEmpty())
        return;

    QDir D(suitePath);
    if (!D.exists())
        return;

    m_unitTestScanList.clear();
    m_systemTestScanList.clear();

    scanPotentialFiles(suitePath);

    if (!m_systemTestScanList.isEmpty())
        m_qmlJSModelManager->updateSourceFiles(m_systemTestScanList, true);
    if (!m_unitTestScanList.isEmpty())
        m_cppModelManager->updateSourceFiles(m_unitTestScanList);
}

void TestCollectionPrivate::scanPotentialFiles(const QString &suitePath)
{
    if (suitePath.isEmpty())
        return;

    QDir D(suitePath);
    if (!D.exists())
        return;

    QFileInfoList qttTestFiles = D.entryInfoList(QStringList(QLatin1String("*.qtt")), QDir::Files);
    foreach (const QFileInfo &qttTestFile, qttTestFiles) {
        m_systemTestScanList.append(qttTestFile.absoluteFilePath());
        m_qttDocumentMap[qttTestFile.absoluteFilePath()] = 0;
    }

    QFileInfoList cppTestFiles = D.entryInfoList(QStringList(QLatin1String("*.cpp")), QDir::Files);
    const CPlusPlus::Snapshot snapshot = m_cppModelManager->snapshot();
    foreach (const QFileInfo &cppTestFile, cppTestFiles) {
        m_cppDocumentMap[cppTestFile.absoluteFilePath()] = 0;
        if (snapshot.contains(cppTestFile.absoluteFilePath())) {
            CPlusPlus::Document::Ptr doc = snapshot.find(cppTestFile.absoluteFilePath()).value();
            onDocumentUpdated(doc);
        } else {
            m_unitTestScanList.append(cppTestFile.absoluteFilePath());
        }
    }

    QStringList potentialSubdirs =
        D.entryList(QStringList(QString(QLatin1Char('*'))), QDir::Dirs|QDir::NoDotAndDotDot);
    foreach (const QString &dname, potentialSubdirs) {
        // stop scanning subdirs if we've ended up in a testdata subdir
        if (dname != QLatin1String("testdata"))
            scanPotentialFiles(suitePath + QDir::separator() + dname);
    }
}

TestCode *TestCollectionPrivate::currentEditedTest()
{
    return m_currentEditedTest;
}

void TestCollectionPrivate::setCurrentEditedTest(TestCode *code)
{
    m_currentEditedTest = code;
}

TestCode *TestCollectionPrivate::findCode(const QString &fileName, const QString &basePath,
    const QString &extraPath)
{
    TestCode *tmp;
    for (int i = 0; i < m_codeList.count(); ++i) {
        tmp = m_codeList.at(i);
        if (tmp && tmp->actualFileName() == fileName)
            return tmp;
    }

    if (!basePath.isEmpty()) {
        QFileInfo inf(fileName);
        if (inf.exists() && inf.isFile()) {
            tmp = new TestCode(basePath, extraPath, fileName);

            if (m_cppDocumentMap[fileName])
                delete m_cppDocumentMap[fileName];

            if (fileName.endsWith(QLatin1String(".qtt"))) {
                m_qttDocumentMap[fileName] = tmp;
                m_qmlJSModelManager->updateSourceFiles(QStringList(fileName), true);
            } else {
                m_cppDocumentMap[fileName] = tmp;
                if (!m_cppModelManager->snapshot().contains(fileName))
                    m_cppModelManager->updateSourceFiles(QStringList(fileName));
            }
            if (tmp) {
                m_codeList.append(tmp);
                connect(tmp, SIGNAL(testChanged(TestCode*)), this, SIGNAL(testChanged(TestCode*)));
                connect(tmp, SIGNAL(testRemoved(TestCode*)), this, SIGNAL(testRemoved(TestCode*)));

                emit testChanged(tmp);
                return tmp;
            }
        }
    }

    return 0;
}

TestCode *TestCollectionPrivate::findCodeByVisibleName(const QString &fileName, bool componentMode)
{
    TestCode *tmp;
    for (int i = 0; i < m_codeList.count(); ++i) {
        tmp = m_codeList.at(i);
        if (tmp && tmp->visualFileName(componentMode) == fileName)
            return tmp;
    }
    return 0;
}

TestCode *TestCollectionPrivate::findCodeByTestCaseName(const QString &testCaseName)
{
    TestCode *tmp;
    for (int i = 0; i < m_codeList.count(); ++i) {
        tmp = m_codeList.at(i);
        if (tmp && tmp->testCase() == testCaseName)
            return tmp;
    }
    return 0;
}

TestCode* TestCollectionPrivate::testCode(int index)
{
    if (index < m_codeList.count())
        return m_codeList.at(index);
    return 0;
}

int TestCollectionPrivate::count()
{
    return m_codeList.count();
}

QStringList TestCollectionPrivate::testFiles()
{
    QStringList ret;
    TestCode *tmp;
    for (int i = 0; i < m_codeList.count(); ++i) {
        tmp = m_codeList.at(i);
        if (tmp)
            ret.append(tmp->actualFileName());
    }
    return ret;
}

QStringList TestCollectionPrivate::manualTests(const QString &startPath, bool componentMode)
{
    QStringList ret;
    TestCode *tmp;
    for (int i = 0; i < m_codeList.count(); ++i) {
        tmp = m_codeList.at(i);
        if (tmp && (startPath.isEmpty() || tmp->visualFileName(componentMode).startsWith(startPath))) {
            for (uint j = 0; j < tmp->testFunctionCount(); ++j) {
                TestFunctionInfo *inf = tmp->testFunction(j);
                if (inf && inf->isManualTest()) {
                    ret.append(tmp->testCase());
                    ret.append(QLatin1String("::"));
                    ret.append(inf->functionName());
                }
            }
        }
    }
    return ret;
}

// ***************************************************************************

TestCollectionPrivate *TestCollection::d = 0;
int TestCollection::m_refCount = 0;

TestCollection::TestCollection()
{
    if (m_refCount++ == 0) {
        d = new TestCollectionPrivate();
    }
    connect(d, SIGNAL(changed()), this, SIGNAL(changed()));
    connect(d, SIGNAL(testChanged(TestCode*)), this, SIGNAL(testChanged(TestCode*)));
    connect(d, SIGNAL(testRemoved(TestCode*)), this, SIGNAL(testRemoved(TestCode*)));
}

TestCollection::~TestCollection()
{
    disconnect(d, SIGNAL(changed()), this, SIGNAL(changed()));
    disconnect(d, SIGNAL(testChanged(TestCode*)), this, SIGNAL(testChanged(TestCode*)));
    disconnect(d, SIGNAL(testRemoved(TestCode*)), this, SIGNAL(testRemoved(TestCode*)));

    if (--m_refCount == 0) {
        delete d;
        d = 0;
    }
}

TestCode *TestCollection::findCode(const QString &fileName, const QString &basePath,
    const QString &extraPath)
{
    Q_ASSERT(d);
    return d->findCode(fileName, basePath, extraPath);
}

TestCode *TestCollection::findCodeByVisibleName(const QString &fileName, bool componentMode)
{
    Q_ASSERT(d);
    return d->findCodeByVisibleName(fileName, componentMode);
}

TestCode *TestCollection::findCodeByTestCaseName(const QString &testCaseName)
{
    Q_ASSERT(d);
    return d->findCodeByTestCaseName(testCaseName);
}

TestCode* TestCollection::testCode(int index)
{
    Q_ASSERT(d);
    return d->testCode(index);
}

int TestCollection::count()
{
    Q_ASSERT(d);
    return d->count();
}

void TestCollection::addPath(const QString &srcPath)
{
    Q_ASSERT(d);
    d->addPath(srcPath);
}

void TestCollection::addExtraPath(const QString &srcPath)
{
    Q_ASSERT(d);
    d->addExtraPath(srcPath);
}

void TestCollection::removePath(const QString &srcPath)
{
    Q_ASSERT(d);
    d->removePath(srcPath);
}

void TestCollection::removeAll()
{
    Q_ASSERT(d);
    d->removeAll();
}

QStringList TestCollection::testFiles()
{
    Q_ASSERT(d);
    return d->testFiles();
}

QStringList TestCollection::manualTests(const QString &startPath, bool componentMode)
{
    Q_ASSERT(d);
    return d->manualTests(startPath, componentMode);
}

TestCode *TestCollection::currentEditedTest()
{
    Q_ASSERT(d);
    return d->currentEditedTest();
}

void TestCollection::setCurrentEditedTest(TestCode *code)
{
    Q_ASSERT(d);
    d->setCurrentEditedTest(code);
}

// ****************************************************************************

TestFunctionInfo *TestCode::processFunction(const QString &name, int startLine, int start, int end)
{
    TestFunctionInfo *tfi = 0;
    if (validFunctionName(name)) {
        tfi = findFunction(name);

        if (!tfi) {
            tfi = new TestFunctionInfo();
            m_testFunctions.append(tfi);
            tfi->setDeclarationStartLine(startLine);
            tfi->setDeclarationStart(start);
            tfi->setDeclarationEnd(end);
        }

        tfi->setTestStartLine(startLine);
        tfi->setTestStart(start);
        tfi->setTestEnd(end);
        tfi->setFunctionName(name);
    }
    return tfi;
}

void TestCode::setManualTest(int offset)
{
    if (TestFunctionInfo *functionInfo = findFunction(offset, false)) {
        functionInfo->setManualTest(true);
    }
}

bool TestCode::validFunctionName(const QString &funcName)
{
    if (funcName.isEmpty() || funcName == QLatin1String("initTestCase") || funcName == QLatin1String("init")
        || funcName == QLatin1String("cleanupTestCase") || funcName == QLatin1String("cleanup") || funcName.endsWith(QLatin1String("_data"))) {
        return false;
    }
    return true;
}

bool TestCode::gotoLine(int lineNumber)
{
    if (lineNumber >= 0 && m_codeEditor) {
        m_codeEditor->gotoLine(lineNumber);
        return true;
    }
    return false;
}

/*!
    Adds a new testfunction \a newFuncName to the class.
*/
void TestCode::addTestFunction(const QString &newFuncName, const QString &newFuncHeader,
    bool insertAtCursorPosition)
{
    Q_UNUSED(newFuncHeader);

    if (!m_codeEditor) return;

    if (m_testType == TypeSystemTest) {
        int entryPoint = 0;
        if (!insertAtCursorPosition) {
            // Insert after last test function
            if (!m_testFunctions.isEmpty())
                entryPoint = m_testFunctions.last()->testEnd() + 1;
        } else {
            if (m_testFunctions.isEmpty()) {
                entryPoint = m_codeEditor->position();
            } else {
                // Insert after test function at cursor (or at end if not in function)
                entryPoint = m_testFunctions.last()->testEnd() + 1;
                int pos = m_codeEditor->position();
                foreach (TestFunctionInfo *func, m_testFunctions) {
                    if ((func->testStart() <= pos && func->testEnd() >= pos) ||
                        func == m_testFunctions.last()) {
                        entryPoint = func->testEnd()+1;
                        break;
                    }
                }
            }
        }

        QString insertString =
            QString::fromLatin1("\n\n    %1_data:\n    {\n    },\n\n    %1: function()\n    {\n    }")
            .arg(newFuncName);

        m_codeEditor->setCursorPosition(entryPoint);
        int lineLength = m_codeEditor->position(TextEditor::ITextEditor::EndOfLine)
            - m_codeEditor->position();
        QString lineBefore = m_codeEditor->textAt(m_codeEditor->position(), lineLength).simplified();
        m_codeEditor->setCursorPosition(m_codeEditor->position(TextEditor::ITextEditor::EndOfLine));

        if (!lineBefore.endsWith(QLatin1Char(',')) && !m_testFunctions.isEmpty())
            insertString.prepend(QLatin1Char(','));
        else
            insertString.append(QLatin1Char(','));

        m_codeEditor->insert(insertString);
    } else {
        unsigned entryLine;
        unsigned declLine;
        if (!insertAtCursorPosition) {
            // Insert after last test function
            if (!m_testFunctions.isEmpty())
                m_cppDoc->translationUnit()->getPosition(m_testFunctions.last()->testEnd(), &entryLine);
            m_cppDoc->translationUnit()->getPosition(m_testFunctions.last()->declarationEnd(),
                &declLine);
        } else {
            if (m_testFunctions.isEmpty()) {
                entryLine = m_codeEditor->currentLine();
                declLine = entryLine; // FIXME where?
            } else {
                // Insert after test function at cursor (or at end if not in function)
                m_cppDoc->translationUnit()->getPosition(m_testFunctions.last()->testEnd(),
                    &entryLine);
                m_cppDoc->translationUnit()->getPosition(m_testFunctions.last()->declarationEnd(),
                    &declLine);
                unsigned posLine = m_codeEditor->currentLine();
                unsigned funcEndLine;
                foreach (TestFunctionInfo *func, m_testFunctions) {
                    if (func->testStartLine() <= m_codeEditor->currentLine()) {
                        m_cppDoc->translationUnit()->getPosition(func->testEnd(), &funcEndLine);
                        if ((funcEndLine >= posLine) || func == m_testFunctions.last()) {
                            entryLine = funcEndLine;
                            m_cppDoc->translationUnit()->getPosition(func->declarationEnd(),
                                &declLine);
                            break;
                        }
                    }
                }
            }
        }
        ++entryLine;
        ++declLine;

        QString insertString =
            QString("\nvoid %1::%2_data()\n{\n}\n\nvoid %1::%2()\n{\n}\n").arg(m_testCase)
            .arg(newFuncName);
        m_codeEditor->gotoLine(entryLine);
        m_codeEditor->insert(insertString);

        //FIXME: What happens if test function is added in class declaration?

        m_codeEditor->gotoLine(declLine);
        m_codeEditor->setCursorPosition(m_codeEditor->position(TextEditor::ITextEditor::StartOfLine));
        insertString = QString::fromLatin1("    void %1_data();\n    void %1();\n").arg(newFuncName);
        m_codeEditor->insert(insertString);
        m_codeEditor->gotoLine(entryLine + 2);
    }
}

void TestCode::save(bool force)
{
    Q_UNUSED(force);
    if (m_codeEditor && m_codeEditor->file()) {
        Core::FileManager *fm = Core::ICore::instance()->fileManager();
        QList<Core::IFile *> files;
        files << m_codeEditor->file();
        fm->saveModifiedFilesSilently(files);
    }
}

QString TestCode::visualBasePath()
{
    return m_basePath;
}

QString TestCode::actualBasePath()
{
    if (!m_externalPath.isEmpty())
        return m_externalPath;
    return m_basePath;
}

QString TestCode::fullVisualSuitePath(bool componentViewMode) const
{
    if (m_fileName.isEmpty())
        return QString();

    if (componentViewMode)
        return QDir::convertSeparators(m_basePath + QDir::separator() + testedComponent());

    if (!m_fileName.endsWith(QLatin1String(".cpp")) && !m_fileName.endsWith(QLatin1String(".qtt")))
        return m_fileName;

    QString fn = m_fileName;
    if (!m_externalPath.isEmpty() && fn.startsWith(m_externalPath)) {
        fn.remove(m_externalPath);
        fn = m_basePath + QDir::separator() + QLatin1String("tests") + QDir::separator() + QLatin1String("external") + fn;
    }

    QFileInfo inf(fn);
    fn = inf.absolutePath(); // try to remove file name

    // remove directory if it's the same as the filename
    if (fn.endsWith(QDir::separator() + inf.baseName()))
        fn = fn.left(fn.length() - (inf.baseName().length() + 1));

    // remove '/tests" if that's on the end
    QString eolstr = QString(QDir::separator()) + QLatin1String("tests");
    if (fn.endsWith(eolstr)) {
        int pos = fn.lastIndexOf(eolstr);
        if (pos) fn = fn.left(pos);
    }
    return QDir::convertSeparators(fn);
}

/*!
    Returns the full filename as it is shown in the Test Selector.
*/

QString TestCode::visualFileName(bool componentViewMode) const
{
    return fullVisualSuitePath(componentViewMode) + QDir::separator() + baseFileName();
}

/*
  Returns the file name without a path, eg: tst_foobar.cpp
*/
QString TestCode::baseFileName() const
{
    return m_fileInfo->fileName();
}

/*!
    Returns the currently opened filename.
*/
QString TestCode::actualFileName() const
{
    return m_fileName;
}

QString TestCode::targetFileName(const QString &buildPath) const
{
    QString fn = m_fileName;
    if (!m_externalPath.isEmpty() && fn.startsWith(m_externalPath)) {
        fn.remove(m_externalPath);
        fn = m_basePath + QDir::separator() + QLatin1String("tests") + QDir::separator()
            + QLatin1String("external") + QDir::separator() + fn;
    }
    fn.remove(m_basePath);

    return QDir::convertSeparators(buildPath + fn);
}

void TestCode::onContextHelpIdRequested(TextEditor::ITextEditor *editor, int position)
{
#ifdef QTTEST_PLUGIN_LEAN
    Q_UNUSED(editor);
    Q_UNUSED(position);
#else
    int charsBefore = 32;
    if (position < charsBefore)
        charsBefore = position;

    QString text = editor->textAt(position - charsBefore, 64);
    int start = 0;
    int end = text.length();

    for (int i = charsBefore; i; --i) {
        if (!text[i].isLetter()) {
            start = i + 1;
            break;
        }
    }
    for (int i = charsBefore; i < end; ++i) {
        if (!text[i].isLetter()) {
            end = i;
            break;
        }
    }

    static QStringList qstSlots;
    if (qstSlots.isEmpty()) {
        for (int i = 0; i < QSystemTest::staticMetaObject.methodCount(); ++i) {
            QString slot(QSystemTest::staticMetaObject.method(i).signature());
            if (!qstSlots.contains(slot))
                qstSlots << slot.left(slot.indexOf('('));
        }
        qstSlots << QLatin1String("compare") << QLatin1String("verify") << QLatin1String("waitFor")
            << QLatin1String("expect") << QLatin1String("fail") << QLatin1String("tabBar") << QLatin1String("menuBar");
    }

    if (start < end) {
        QString function = text.mid(start, end - start);
        if (qstSlots.contains(function)) {
            m_codeEditor->setContextHelpId(QString::fromLatin1("QSystemTest::%1").arg(function));
            return;
        }
    }
#endif

    m_codeEditor->setContextHelpId(QLatin1String("QtUiTest Manual"));
}

bool TestCode::hasUnsavedChanges() const
{
    if (m_codeEditor && m_codeEditor->file()) {
        return m_codeEditor->file()->isModified();
    }
    return false;
}

bool TestCode::isInitialized() const
{
    return m_initialized;
}

bool TestCode::isErrored() const
{
    return m_errored;
}
