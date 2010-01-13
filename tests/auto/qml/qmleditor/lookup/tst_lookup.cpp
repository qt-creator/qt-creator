#include <QDebug>
#include <QtTest>
#include <QObject>
#include <QFile>

#include <qml/qmldocument.h>
#include <qml/parser/qmljsast_p.h>

#include <qmllookupcontext.h>

#include <typeinfo>

using namespace Qml;
using namespace QmlEditor;
using namespace QmlEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

class tst_Lookup: public QObject
{
    Q_OBJECT

public:
    tst_Lookup(): _typeSystem(0) {}
    ~tst_Lookup() { if (_typeSystem) resetTypeSystem(); }

    void resetTypeSystem() { if (_typeSystem) { delete _typeSystem; _typeSystem = 0; }}

private Q_SLOTS:
    void basicSymbolTest();
    void basicLookupTest();
    void localIdLookup();
    void localScriptMethodLookup();
    void localScopeLookup();
    void localRootLookup();

protected:
    QmlDocument::Ptr basicSymbolTest(const QString &input) const
    {
        const QLatin1String filename("<lookup test>");
        QmlDocument::Ptr doc = QmlDocument::create(filename);
        doc->setSource(input);
        doc->parse();

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

    Snapshot snapshot(const QmlDocument::Ptr &doc) const
    {
        Snapshot snapshot;
        snapshot.insert(doc);
        return snapshot;
    }

    QmlTypeSystem *typeSystem() {
        if (!_typeSystem)
            _typeSystem = new QmlTypeSystem;
        return _typeSystem;
    }

private:
    QmlTypeSystem *_typeSystem;
};

void tst_Lookup::basicSymbolTest()
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

    QmlDocument::Ptr doc = basicSymbolTest(input);
    QVERIFY(doc->isParsedCorrectly());

    UiProgram *program = doc->program();
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

    QmlSymbol::List docSymbols = doc->symbols();
    QCOMPARE(docSymbols.size(), 1);

    QmlSymbol *rectSymbol = docSymbols.at(0);
    QCOMPARE(rectSymbol->name(), QLatin1String("Rectangle"));
    QCOMPARE(rectSymbol->members().size(), 4);

    QmlSymbolFromFile *rectFromFile = rectSymbol->asSymbolFromFile();
    QVERIFY(rectFromFile);
    QmlSymbolFromFile *xSymbol = rectFromFile->findMember(xBinding);
    QVERIFY(xSymbol);
    QCOMPARE(xSymbol->name(), QLatin1String("x"));
}

void tst_Lookup::basicLookupTest()
{
    const QLatin1String input(
            "import Qt 4.6\n"
            "Item{}\n"
            );

    QmlDocument::Ptr doc = basicSymbolTest(input);
    QVERIFY(doc->isParsedCorrectly());

    UiProgram *program = doc->program();
    QVERIFY(program);

    QStack<QmlSymbol *> emptyScope;
    QmlLookupContext context(emptyScope, doc, snapshot(doc), typeSystem());
    QmlSymbol *rectSymbol = context.resolveType(QLatin1String("Text"));
    QVERIFY(rectSymbol);

    QmlBuildInSymbol *buildInRect = rectSymbol->asBuildInSymbol();
    QVERIFY(buildInRect);
    QCOMPARE(buildInRect->name(), QLatin1String("Text"));

    QmlSymbol::List allBuildInRectMembers = buildInRect->members(true);
    QVERIFY(!allBuildInRectMembers.isEmpty());
    bool xPropFound = false;
    bool fontPropFound = false;
    foreach (QmlSymbol *symbol, allBuildInRectMembers) {
        if (symbol->name() == QLatin1String("x"))
            xPropFound = true;
        else if (symbol->name() == QLatin1String("font"))
            fontPropFound = true;
    }
    QVERIFY(xPropFound);
    QVERIFY(fontPropFound);

    QmlSymbol::List buildInRectMembers = buildInRect->members(false);
    QVERIFY(!buildInRectMembers.isEmpty());

    QSKIP("Getting properties _without_ the inerited properties doesn't work.", SkipSingle);
    fontPropFound = false;
    foreach (QmlSymbol *symbol, buildInRectMembers) {
        if (symbol->name() == QLatin1String("x"))
            QFAIL("Text has x property");
        else if (symbol->name() == QLatin1String("font"))
            fontPropFound = true;
    }
    QVERIFY(fontPropFound);
}

void tst_Lookup::localIdLookup()
{
    QFile input(":/data/localIdLookup.qml");
    QVERIFY(input.open(QIODevice::ReadOnly));

    QmlDocument::Ptr doc = basicSymbolTest(input.readAll());
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
    QStack<QmlSymbol *> scopes;
    foreach (const QString &contextSymbolName, symbolNames) {
        scopes.push_back(doc->ids()[contextSymbolName]->parentNode());
        QmlLookupContext context(scopes, doc, snapshot(doc), typeSystem());

        foreach (const QString &lookupSymbolName, symbolNames) {
            QmlSymbol *resolvedSymbol = context.resolve(lookupSymbolName);
            QmlIdSymbol *targetSymbol = doc->ids()[lookupSymbolName];
            QCOMPARE(resolvedSymbol, targetSymbol);

            QmlIdSymbol *resolvedId = resolvedSymbol->asIdSymbol();
            QVERIFY(resolvedId);
            QCOMPARE(resolvedId->parentNode(), targetSymbol->parentNode());
        }
    }
}

void tst_Lookup::localScriptMethodLookup()
{
    QFile input(":/data/localScriptMethodLookup.qml");
    QVERIFY(input.open(QIODevice::ReadOnly));

    QmlDocument::Ptr doc = basicSymbolTest(input.readAll());
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
    QStack<QmlSymbol *> scopes;
    foreach (const QString &contextSymbolName, symbolNames) {
        scopes.push_back(doc->ids()[contextSymbolName]->parentNode());
        QmlLookupContext context(scopes, doc, snapshot(doc), typeSystem());

        foreach (const QString &functionName, functionNames) {
            QmlSymbol *symbol = context.resolve(functionName);
            QVERIFY(symbol);
            QVERIFY(!symbol->isProperty());
            // verify that it's a function
        }
    }
}

void tst_Lookup::localScopeLookup()
{
    QFile input(":/data/localScopeLookup.qml");
    QVERIFY(input.open(QIODevice::ReadOnly));

    QmlDocument::Ptr doc = basicSymbolTest(input.readAll());
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
    QStack<QmlSymbol *> scopes;
    foreach (const QString &contextSymbolName, symbolNames) {
        QmlSymbol *parent = doc->ids()[contextSymbolName]->parentNode();
        scopes.push_back(parent);
        QmlLookupContext context(scopes, doc, snapshot(doc), typeSystem());

        QmlSymbol *symbol;
        symbol = context.resolve("prop");
        QVERIFY(symbol);
        QVERIFY(symbol->isPropertyDefinitionSymbol());
        QVERIFY(parent->members().contains(symbol));

        symbol = context.resolve("x");
        QVERIFY(symbol);
        QVERIFY(symbol->isProperty());
        QVERIFY(parent->members().contains(symbol));
    }
}

void tst_Lookup::localRootLookup()
{
    QFile input(":/data/localRootLookup.qml");
    QVERIFY(input.open(QIODevice::ReadOnly));

    QmlDocument::Ptr doc = basicSymbolTest(input.readAll());
    QVERIFY(doc->isParsedCorrectly());

    QStringList symbolNames;
    symbolNames.append("theRoot");
    symbolNames.append("theParent");
    symbolNames.append("theChild");

    // check symbol existence and build scopes
    QStack<QmlSymbol *> scopes;
    foreach (const QString &symbolName, symbolNames) {
        QmlIdSymbol *id = doc->ids()[symbolName];
        QVERIFY(id);
        scopes.push_back(id->parentNode());
    }

    // try lookup
    QmlSymbol *parent = scopes.top();
    QmlLookupContext context(scopes, doc, snapshot(doc), typeSystem());
    
    QmlSymbol *symbol;
    symbol = context.resolve("prop");
    QVERIFY(symbol);
    QVERIFY(symbol->isPropertyDefinitionSymbol());
    QVERIFY(parent->members().contains(symbol));

    symbol = context.resolve("color");
    QVERIFY(symbol);
    QVERIFY(symbol->isProperty());
    QVERIFY(parent->members().contains(symbol));
}

QTEST_APPLESS_MAIN(tst_Lookup)
#include "tst_lookup.moc"
