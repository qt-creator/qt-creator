/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <QDebug>
#include <QtTest>
#include <QObject>
#include <QFile>

#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsast_p.h>

#include <qmllookupcontext.h>

#include <typeinfo>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

class tst_QMLLookup: public QObject
{
    Q_OBJECT

public:
    tst_QMLLookup(): _typeSystem(0) {}
    ~tst_QMLLookup() { if (_typeSystem) resetTypeSystem(); }

    void resetTypeSystem() { if (_typeSystem) { delete _typeSystem; _typeSystem = 0; }}

private Q_SLOTS:
    void basicSymbolTest();
    void basicLookupTest();
    void localIdLookup();
    void localScriptMethodLookup();
    void localScopeLookup();
    void localRootLookup();

protected:
    Document::Ptr basicSymbolTest(const QString &input) const
    {
        const QLatin1String filename("<lookup test>");
        Document::Ptr doc = Document::create(filename);
        doc->setSource(input);
        doc->parseQml();

        QList<DiagnosticMessage> msgs = doc->diagnosticMessages();
        foreach (const DiagnosticMessage &msg, msgs) {
            if (msg.isError()) {
                qDebug() << "Error:" << filename << ":" << msg.loc.startLine << ":" << msg.loc.startColumn << ":" << msg.message;
            } else if (msg.isWarning()) {
                qDebug() << "Warning:" << filename << ":" << msg.loc.startLine << ":" << msg.loc.startColumn << ":" << msg.message;
            } else {
                qDebug() << "Diagnostic:" << filename << ":" << msg.loc.startLine << ":" << msg.loc.startColumn << ":" << msg.message;
            }
        }

        return doc;
    }

    Snapshot snapshot(const Document::Ptr &doc) const
    {
        Snapshot snapshot;
        snapshot.insert(doc);
        return snapshot;
    }

    TypeSystem *typeSystem() {
        if (!_typeSystem)
            _typeSystem = new TypeSystem;
        return _typeSystem;
    }

private:
    TypeSystem *_typeSystem;
};

void tst_QMLLookup::basicSymbolTest()
{
    const QLatin1String input(
            "import Qt 4.6\n"
            "\n"
            "Rectangle {\n"
            "    x: 10\n"
            "    y: 10\n"
            "    width: 10\n"
            "    height: 10\n"
            "}\n"
            );

    Document::Ptr doc = basicSymbolTest(input);
    QVERIFY(doc->isParsedCorrectly());

    UiProgram *program = doc->qmlProgram();
    QVERIFY(program);
    QVERIFY(program->members);
    QVERIFY(program->members->member);
    UiObjectDefinition *rectDef = cast<UiObjectDefinition*>(program->members->member);
    QVERIFY(rectDef);
    QVERIFY(rectDef->qualifiedTypeNameId->name);
    QCOMPARE(rectDef->qualifiedTypeNameId->name->asString(), QLatin1String("Rectangle"));
    QVERIFY(rectDef->initializer);
    UiObjectMemberList *rectMembers = rectDef->initializer->members;
    QVERIFY(rectMembers);
    QVERIFY(rectMembers->member);

    UiScriptBinding *xBinding = cast<UiScriptBinding*>(rectMembers->member);
    QVERIFY(xBinding);
    QVERIFY(xBinding->qualifiedId);
    QVERIFY(xBinding->qualifiedId->name);
    QCOMPARE(xBinding->qualifiedId->name->asString(), QLatin1String("x"));

    Symbol::List docSymbols = doc->symbols();
    QCOMPARE(docSymbols.size(), 1);

    Symbol *rectSymbol = docSymbols.at(0);
    QCOMPARE(rectSymbol->name(), QLatin1String("Rectangle"));
    QCOMPARE(rectSymbol->members().size(), 4);

    SymbolFromFile *rectFromFile = rectSymbol->asSymbolFromFile();
    QVERIFY(rectFromFile);
    SymbolFromFile *xSymbol = rectFromFile->findMember(xBinding);
    QVERIFY(xSymbol);
    QCOMPARE(xSymbol->name(), QLatin1String("x"));
}

void tst_QMLLookup::basicLookupTest()
{
    const QLatin1String input(
            "import Qt 4.6\n"
            "Item{}\n"
            );

    Document::Ptr doc = basicSymbolTest(input);
    QVERIFY(doc->isParsedCorrectly());

    UiProgram *program = doc->qmlProgram();
    QVERIFY(program);

    QStack<Symbol *> emptyScope;
    QmlLookupContext context(emptyScope, doc, snapshot(doc), typeSystem());
    Symbol *rectSymbol = context.resolveType(QLatin1String("Text"));
    QVERIFY(rectSymbol);

    PrimitiveSymbol *buildInRect = rectSymbol->asPrimitiveSymbol();
    QVERIFY(buildInRect);
    QCOMPARE(buildInRect->name(), QLatin1String("Text"));

    Symbol::List allBuildInRectMembers = buildInRect->members(true);
    QVERIFY(!allBuildInRectMembers.isEmpty());
    bool xPropFound = false;
    bool fontPropFound = false;
    foreach (Symbol *symbol, allBuildInRectMembers) {
        if (symbol->name() == QLatin1String("x"))
            xPropFound = true;
        else if (symbol->name() == QLatin1String("font"))
            fontPropFound = true;
    }
    QVERIFY(xPropFound);
    QVERIFY(fontPropFound);

    Symbol::List buildInRectMembers = buildInRect->members(false);
    QVERIFY(!buildInRectMembers.isEmpty());

    QSKIP("Getting properties _without_ the inerited properties doesn't work.", SkipSingle);
    fontPropFound = false;
    foreach (Symbol *symbol, buildInRectMembers) {
        if (symbol->name() == QLatin1String("x"))
            QFAIL("Text has x property");
        else if (symbol->name() == QLatin1String("font"))
            fontPropFound = true;
    }
    QVERIFY(fontPropFound);
}

void tst_QMLLookup::localIdLookup()
{
    QFile input(":/data/localIdLookup.qml");
    QVERIFY(input.open(QIODevice::ReadOnly));

    Document::Ptr doc = basicSymbolTest(input.readAll());
    QVERIFY(doc->isParsedCorrectly());

    QStringList symbolNames;
    symbolNames.append("x");
    symbolNames.append("y");
    symbolNames.append("z");
    symbolNames.append("opacity");
    symbolNames.append("visible");

    // check symbol existence
    foreach (const QString &symbolName, symbolNames) {
        QVERIFY(doc->ids()[symbolName]);
    }

    // try lookup
    QStack<Symbol *> scopes;
    foreach (const QString &contextSymbolName, symbolNames) {
        scopes.push_back(doc->ids()[contextSymbolName]->parentNode());
        QmlLookupContext context(scopes, doc, snapshot(doc), typeSystem());

        foreach (const QString &lookupSymbolName, symbolNames) {
            Symbol *resolvedSymbol = context.resolve(lookupSymbolName);
            IdSymbol *targetSymbol = doc->ids()[lookupSymbolName];
            QCOMPARE(resolvedSymbol, targetSymbol);

            IdSymbol *resolvedId = resolvedSymbol->asIdSymbol();
            QVERIFY(resolvedId);
            QCOMPARE(resolvedId->parentNode(), targetSymbol->parentNode());
        }
    }
}

void tst_QMLLookup::localScriptMethodLookup()
{
    QFile input(":/data/localScriptMethodLookup.qml");
    QVERIFY(input.open(QIODevice::ReadOnly));

    Document::Ptr doc = basicSymbolTest(input.readAll());
    QVERIFY(doc->isParsedCorrectly());

    QStringList symbolNames;
    symbolNames.append("theRoot");
    symbolNames.append("theParent");
    symbolNames.append("theChild");

    QStringList functionNames;
    functionNames.append("x");
    functionNames.append("y");
    functionNames.append("z");

    // check symbol existence
    foreach (const QString &symbolName, symbolNames) {
        QVERIFY(doc->ids()[symbolName]);
    }

    // try lookup
    QStack<Symbol *> scopes;
    foreach (const QString &contextSymbolName, symbolNames) {
        scopes.push_back(doc->ids()[contextSymbolName]->parentNode());
        QmlLookupContext context(scopes, doc, snapshot(doc), typeSystem());

        foreach (const QString &functionName, functionNames) {
            Symbol *symbol = context.resolve(functionName);
            QVERIFY(symbol);
            QVERIFY(!symbol->isProperty());
            // verify that it's a function
        }
    }
}

void tst_QMLLookup::localScopeLookup()
{
    QFile input(":/data/localScopeLookup.qml");
    QVERIFY(input.open(QIODevice::ReadOnly));

    Document::Ptr doc = basicSymbolTest(input.readAll());
    QVERIFY(doc->isParsedCorrectly());

    QStringList symbolNames;
    symbolNames.append("theRoot");
    symbolNames.append("theParent");
    symbolNames.append("theChild");

    // check symbol existence
    foreach (const QString &symbolName, symbolNames) {
        QVERIFY(doc->ids()[symbolName]);
    }

    // try lookup
    QStack<Symbol *> scopes;
    foreach (const QString &contextSymbolName, symbolNames) {
        Symbol *parent = doc->ids()[contextSymbolName]->parentNode();
        scopes.push_back(parent);
        QmlLookupContext context(scopes, doc, snapshot(doc), typeSystem());

        Symbol *symbol;
        symbol = context.resolve("prop");
        QVERIFY(symbol);
        QVERIFY(symbol->isPropertyDefinitionSymbol());
        QVERIFY(parent->members().contains(symbol));

        symbol = context.resolve("x");
        QVERIFY(symbol);
        QVERIFY(symbol->isProperty());
        // how do we know we got the right x?
    }
}

void tst_QMLLookup::localRootLookup()
{
    QFile input(":/data/localRootLookup.qml");
    QVERIFY(input.open(QIODevice::ReadOnly));

    Document::Ptr doc = basicSymbolTest(input.readAll());
    QVERIFY(doc->isParsedCorrectly());

    QStringList symbolNames;
    symbolNames.append("theRoot");
    symbolNames.append("theParent");
    symbolNames.append("theChild");

    // check symbol existence and build scopes
    QStack<Symbol *> scopes;
    foreach (const QString &symbolName, symbolNames) {
        IdSymbol *id = doc->ids()[symbolName];
        QVERIFY(id);
        scopes.push_back(id->parentNode());
    }

    // try lookup
    Symbol *parent = scopes.front();
    QmlLookupContext context(scopes, doc, snapshot(doc), typeSystem());

    Symbol *symbol;
    symbol = context.resolve("prop");
    QVERIFY(symbol);
    QVERIFY(symbol->isPropertyDefinitionSymbol());
    QVERIFY(parent->members().contains(symbol));

    symbol = context.resolve("color");
    QVERIFY(symbol);
    QVERIFY(symbol->isProperty());
}

QTEST_APPLESS_MAIN(tst_Lookup)
#include "tst_qmllookup.moc"
