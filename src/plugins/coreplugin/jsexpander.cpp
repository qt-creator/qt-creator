// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsexpander.h"

#include "corejsextensions.h"
#include "coreplugintr.h"
#include "jsbind.h"

#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <quickjs.h>

#include <QDebug>
#include <QDeadlineTimer>
#include <QThread>
#include <QVariant>

#include <chrono>
#include <functional>
#include <unordered_map>

using FunctionExtensionMap = std::unordered_map<QString, Core::JsExpander::FunctionFactory>;
Q_GLOBAL_STATIC(FunctionExtensionMap, globalJsFunctionExtensions);

static Core::JsExpander *globalExpander = nullptr;

namespace Core {
namespace Internal {

// A fully marshalled JavaScript call, stored in the engine and dispatched by
// index from the C trampoline.
using JsThunk = std::function<JSValue(JSContext *, int argc, JSValueConst *argv)>;

class JsExpanderPrivate
{
public:
    JsExpanderPrivate();
    ~JsExpanderPrivate();
    Q_DISABLE_COPY_MOVE(JsExpanderPrivate)

    static constexpr std::chrono::milliseconds s_evalTimeout{200};
    // Fits the smallest thread stack Creator runs on (512 KB on macOS).
    static constexpr size_t s_maxStackSize = 256 * 1024;

    JSRuntime *m_runtime = nullptr;
    JSContext *m_context = nullptr;
    QThread *m_thread = QThread::currentThread();
    QList<JsThunk> m_thunks;
    // Guards evaluate() against never-ending scripts. The interrupt handler is
    // polled by QuickJS during execution and aborts once the deadline of the
    // outermost (possibly nested) evaluation has passed.
    int m_evalDepth = 0;
    QDeadlineTimer m_deadline = QDeadlineTimer::Forever;
};

static int interruptHandler(JSRuntime *, void *opaque)
{
    return static_cast<JsExpanderPrivate *>(opaque)->m_deadline.hasExpired() ? 1 : 0;
}

JsExpanderPrivate::JsExpanderPrivate()
    : m_runtime(JS_NewRuntime())
    , m_context(JS_NewContext(m_runtime))
{
    QTC_CHECK(m_runtime && m_context);
    JS_SetContextOpaque(m_context, this);
    JS_SetInterruptHandler(m_runtime, &interruptHandler, this);
    JS_SetMaxStackSize(m_runtime, s_maxStackSize);
}

JsExpanderPrivate::~JsExpanderPrivate()
{
    JS_FreeContext(m_context);
    JS_FreeRuntime(m_runtime);
}

static QString stringFromJs(JSContext *ctx, JSValueConst value)
{
    const char *utf8 = JS_ToCString(ctx, value);
    const QString string = QString::fromUtf8(utf8);
    JS_FreeCString(ctx, utf8);
    return string;
}

// Converts a JS value to a QVariant, mapping it to the closest Qt type. This is
// the argument side of the QVariant boundary used by the binding layer.
static QVariant jsToVariant(JSContext *ctx, JSValueConst value)
{
    if (JS_IsBool(value))
        return bool(JS_ToBool(ctx, value));
    if (JS_IsNumber(value)) {
        double number = 0;
        JS_ToFloat64(ctx, &number, value);
        return number;
    }
    if (JS_IsString(value))
        return stringFromJs(ctx, value);
    if (JS_IsArray(value)) {
        QVariantList list;
        uint32_t length = 0;
        const JSValue lengthValue = JS_GetPropertyStr(ctx, value, "length");
        JS_ToUint32(ctx, &length, lengthValue);
        JS_FreeValue(ctx, lengthValue);
        for (uint32_t i = 0; i < length; ++i) {
            const JSValue element = JS_GetPropertyUint32(ctx, value, i);
            list.append(jsToVariant(ctx, element));
            JS_FreeValue(ctx, element);
        }
        return list;
    }
    if (JS_IsUndefined(value) || JS_IsNull(value))
        return {};
    // Plain objects fall back to their string form.
    return stringFromJs(ctx, value);
}

// Converts a QVariant to a JS value; the result side of the QVariant boundary.
static JSValue variantToJs(JSContext *ctx, const QVariant &variant)
{
    switch (variant.metaType().id()) {
    case QMetaType::UnknownType:
    case QMetaType::Void:
        return JS_UNDEFINED;
    case QMetaType::Bool:
        return JS_NewBool(ctx, variant.toBool());
    case QMetaType::Int:
        return JS_NewInt32(ctx, variant.toInt());
    case QMetaType::UInt:
        return JS_NewUint32(ctx, variant.toUInt());
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        return JS_NewInt64(ctx, variant.toLongLong());
    case QMetaType::Double:
    case QMetaType::Float:
        return JS_NewFloat64(ctx, variant.toDouble());
    case QMetaType::QStringList: {
        const QStringList list = variant.toStringList();
        JSValue array = JS_NewArray(ctx);
        for (int i = 0; i < list.size(); ++i)
            JS_SetPropertyUint32(ctx, array, i, JS_NewString(ctx, list.at(i).toUtf8().constData()));
        return array;
    }
    case QMetaType::QVariantList: {
        const QVariantList list = variant.toList();
        JSValue array = JS_NewArray(ctx);
        for (int i = 0; i < list.size(); ++i)
            JS_SetPropertyUint32(ctx, array, i, variantToJs(ctx, list.at(i)));
        return array;
    }
    case QMetaType::QVariantMap: {
        const QVariantMap map = variant.toMap();
        JSValue object = JS_NewObject(ctx);
        for (auto it = map.cbegin(); it != map.cend(); ++it)
            JS_SetPropertyStr(ctx, object, it.key().toUtf8().constData(),
                              variantToJs(ctx, it.value()));
        return object;
    }
    default:
        return JS_NewString(ctx, variant.toString().toUtf8().constData());
    }
}

// Trampoline invoked from JS; dispatches to the thunk stored at the index
// carried in the function's data.
static JSValue callThunk(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv,
                         int, JSValueConst *funcData)
{
    auto *d = static_cast<JsExpanderPrivate *>(JS_GetContextOpaque(ctx));
    int32_t index = 0;
    JS_ToInt32(ctx, &index, funcData[0]);
    QTC_ASSERT(index >= 0 && index < d->m_thunks.size(), return JS_UNDEFINED);
    return d->m_thunks.at(index)(ctx, argc, argv);
}

} // namespace Internal

void JsExpander::registerGlobalFunctions(const QString &name, const FunctionFactory &factory)
{
    globalJsFunctionExtensions->insert({name, factory});
    if (globalExpander)
        globalExpander->registerFunctions(name, factory);
}

void JsExpander::registerFunctions(const QString &name, const FunctionFactory &factory)
{
    QTC_ASSERT(QThread::currentThread() == d->m_thread, return);
    JSContext *ctx = d->m_context;
    JSValue object = JS_NewObject(ctx);

    Internal::JsExpanderPrivate *p = d;
    // The sink turns each QVariant-based callable into an engine function; the
    // JS<->QVariant marshalling (and thus QuickJS) stays confined here.
    JsBind::Registrar registrar(
        [ctx, object, p](const char *functionName, JsBind::VariantFunction fn) {
            const int index = p->m_thunks.size();
            p->m_thunks.append(
                [fn = std::move(fn)](JSContext *c, int argc, JSValueConst *argv) -> JSValue {
                    QVariantList args;
                    args.reserve(argc);
                    for (int i = 0; i < argc; ++i)
                        args.append(Internal::jsToVariant(c, argv[i]));
                    return Internal::variantToJs(c, fn(args));
                });
            JSValue data = JS_NewInt32(ctx, index);
            const JSValue function = JS_NewCFunctionData(ctx, &Internal::callThunk, 0, 0, 1, &data);
            JS_FreeValue(ctx, data);
            JS_SetPropertyStr(ctx, object, functionName, function);
        });
    factory(registrar);

    const JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, name.toUtf8().constData(), object);
    JS_FreeValue(ctx, global);
}

QString JsExpander::evaluate(const QString &expression, QString *errorMessage)
{
    QTC_ASSERT(QThread::currentThread() == d->m_thread, {
        if (errorMessage)
            *errorMessage = Tr::tr("JavaScript cannot be evaluated outside the thread of its engine.");
        return {};
    });
    JSContext *ctx = d->m_context;
    if (d->m_evalDepth++ == 0) {
        d->m_deadline = QDeadlineTimer(Internal::JsExpanderPrivate::s_evalTimeout);
        JS_UpdateStackTop(d->m_runtime);
    }
    const QByteArray utf8 = expression.toUtf8();
    const JSValue value = JS_Eval(ctx, utf8.constData(), utf8.size(),
                                  "<expression>", JS_EVAL_TYPE_GLOBAL);
    --d->m_evalDepth;

    QString result;
    if (JS_IsException(value)) {
        const JSValue exception = JS_GetException(ctx);
        const QString msg = Tr::tr("Error in \"%1\": %2")
                                .arg(expression, Internal::stringFromJs(ctx, exception));
        JS_FreeValue(ctx, exception);
        if (errorMessage)
            *errorMessage = msg;
    } else if (JS_IsBool(value) || JS_IsNumber(value) || JS_IsString(value)) {
        result = Internal::stringFromJs(ctx, value);
    } else {
        const QString msg = Tr::tr("Cannot convert result of \"%1\" to string.").arg(expression);
        if (errorMessage)
            *errorMessage = msg;
    }
    JS_FreeValue(ctx, value);
    return result;
}

void JsExpander::registerForExpander(Utils::MacroExpander *macroExpander)
{
    macroExpander->registerPrefix(
        "JS",
        "1+1",
        Tr::tr(
            "Evaluate simple JavaScript statements.<br>"
            "Literal '}' characters must be escaped as \"\\}\", "
            "'\\' characters must be escaped as \"\\\\\", "
            "and \"%{\" must be escaped as \"%\\{\"."),
        [this](QString in) -> QString {
            QString errorMessage;
            QString result = evaluate(in, &errorMessage);
            if (!errorMessage.isEmpty()) {
                qWarning() << errorMessage;
                return errorMessage;
            } else {
                return result;
            }
        });
}

JsExpander *JsExpander::createGlobalJsExpander()
{
    globalExpander = new JsExpander();
    registerGlobalFunctions("Util", &Internal::registerUtilFunctions);
    globalExpander->registerForExpander(Utils::globalMacroExpander());
    return globalExpander;
}

JsExpander::JsExpander()
{
    d = new Internal::JsExpanderPrivate;
    for (const std::pair<const QString, FunctionFactory> &fns : *globalJsFunctionExtensions)
        registerFunctions(fns.first, fns.second);
}

JsExpander::~JsExpander()
{
    delete d;
    d = nullptr;
}

} // namespace Core


#ifdef WITH_TESTS

#include <QTest>

namespace Core::Internal {

class JsExpanderTest final : public QObject
{
    Q_OBJECT

private slots:
    void testBindingConversions_data();
    void testBindingConversions();
    void testExpanderStaysQuietWhenDestroyed();
    void testRunawayRecursionIsAnError();
};

void JsExpanderTest::testBindingConversions_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<QString>("expected");

    QTest::newRow("string") << "T.str('abc')" << "abc";
    QTest::newRow("undefined-argument") << "T.str(undefined)" << "";
    QTest::newRow("missing-argument") << "T.str()" << "";
    QTest::newRow("bool") << "T.negate(true)" << "false";
    QTest::newRow("int") << "T.twice(21)" << "42";
    QTest::newRow("list-in-and-out") << "T.reverse(['a', 'b', 'c']).join(',')" << "c,b,a";
    QTest::newRow("variant-list") << "T.mixed().length" << "3";
    QTest::newRow("variant-map") << "T.map().key" << "value";
    QTest::newRow("nested-evaluate") << "T.nested() + 1" << "31";
    QTest::newRow("fileName") << "Util.fileName('/a/b/foo.txt')" << "foo.txt";
    QTest::newRow("fileName-extension") << "Util.fileName('/a/b/foo', 'txt')" << "/a/b/foo.txt";
}

void JsExpanderTest::testBindingConversions()
{
    QFETCH(QString, expression);
    QFETCH(QString, expected);

    JsExpander expander;
    JsExpander *e = &expander;
    expander.registerFunctions("T", [e](JsBind::Registrar &registrar) {
        registrar.set("str", [](QString in) { return in; });
        registrar.set("negate", [](bool in) { return !in; });
        registrar.set("twice", [](int in) { return in * 2; });
        registrar.set("reverse", [](QStringList in) {
            return QStringList(in.crbegin(), in.crend());
        });
        registrar.set("mixed", [] { return QVariantList{1, "two", true}; });
        registrar.set("map", [] { return QVariantMap{{"key", "value"}}; });
        registrar.set("nested", [e] { return e->evaluate("1 + 2"); });
    });

    QString errorMessage;
    QCOMPARE(expander.evaluate(expression, &errorMessage), expected);
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));
}

void JsExpanderTest::testRunawayRecursionIsAnError()
{
    JsExpander expander;
    QString errorMessage;
    expander.evaluate("(function f() { f(); })()", &errorMessage);
    QVERIFY(!errorMessage.isEmpty());
    QCOMPARE(expander.evaluate("1 + 1"), QString("2"));
}

// A JsExpander is created, used and destroyed right away in several places, most
// notably by every wizard factory that the "New File"/"New Project" dialog
// filters. Destroying it must work, also after an evaluation was cut short.
void JsExpanderTest::testExpanderStaysQuietWhenDestroyed()
{
    for (int i = 0; i < 5; ++i) {
        JsExpander expander;
        QString errorMessage;
        // An endless script is interrupted, and the expander stays usable.
        expander.evaluate("for (;;) {}", &errorMessage);
        QVERIFY(!errorMessage.isEmpty());
        QCOMPARE(expander.evaluate("1 + 1"), QString("2"));
    }
}

QObject *createJsExpanderTest()
{
    return new JsExpanderTest;
}

} // namespace Core::Internal

#endif // WITH_TESTS

#include "jsexpander.moc"
