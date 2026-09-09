// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsexpander.h"

#include "corejsextensions.h"
#include "coreplugintr.h"

#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QJSEngine>
#include <QRecursiveMutex>
#include <QThread>
#include <QTimer>

#include <chrono>
#include <unordered_map>

using ExtensionMap = std::unordered_map<QString, Core::JsExpander::ObjectFactory>;
Q_GLOBAL_STATIC(ExtensionMap, globalJsExtensions);

static Core::JsExpander *globalExpander = nullptr;

namespace Core {
namespace Internal {

// Guards QJSEngine::evaluate() calls against never-ending scripts. watch() must be called right
// before evaluate() and unwatch() right after it returns. Calls may nest (e.g. a JS extension
// object implicitly triggering another %{JS:...} macro expansion), so only the outermost watch()/
// unwatch() pair actually starts/stops the guard. If the evaluate() call doesn't
// finish within evalTimeout, the engine is interrupted. The watchdog thread is started on demand
// by the first watch() call.
class JsEngineWatchdog : public QThread
{
public:
    JsEngineWatchdog()
    {
        setObjectName("JsEngineWatchdog");
        m_evalTimer.setSingleShot(true);
        m_evalTimer.setInterval(s_evalTimeout);
        connect(&m_evalTimer, &QTimer::timeout, &m_evalTimer, [this] {
            if (m_engine)
                m_engine->setInterrupted(true);
        });
        m_evalTimer.moveToThread(this);
    }

    ~JsEngineWatchdog() override
    {
        // Quitting drops the queued unwatch(), which would leave the timer running.
        if (isRunning()) {
            QMetaObject::invokeMethod(
                &m_evalTimer, [this] { stopWatching(); }, Qt::BlockingQueuedConnection);
        }
        quit();
        wait();
    }

    void watch(QJSEngine *engine)
    {
        if (m_depth++ > 0)
            return; // Nested call, the outer guard is already running.
        // Un-interrupt the engine from a previous timeout. Only done for the outermost call, so
        // that this can't undo an interruption of an evaluate() this call is nested in.
        engine->setInterrupted(false);
        if (!isRunning())
            start();
        // use timer for thread-affinity, since JsEngineWatchdog itself "lives" in the main thread
        QMetaObject::invokeMethod(
            &m_evalTimer, [this, engine] { startWatching(engine); }, Qt::QueuedConnection);
    }

    void unwatch()
    {
        QTC_ASSERT(m_depth > 0, return); // unwatch() without a matching watch()
        if (--m_depth > 0)
            return; // Still inside an outer evaluate() call.
        // use timer for thread-affinity, since JsEngineWatchdog itself "lives" in the main thread
        QMetaObject::invokeMethod(&m_evalTimer, [this] { stopWatching(); }, Qt::QueuedConnection);
    }

private:
    // The following two run in the watchdog thread.
    void startWatching(QJSEngine *engine)
    {
        m_engine = engine;
        m_evalTimer.start();
    }

    void stopWatching()
    {
        m_evalTimer.stop();
        m_engine = nullptr;
    }

    static constexpr std::chrono::milliseconds s_evalTimeout{200};

    int m_depth = 0;
    QTimer m_evalTimer;
    QJSEngine *m_engine = nullptr;
};

class JsExpanderPrivate {
public:
    QJSEngine m_engine;
    // QJSEngine is not thread-safe, but the global expander's JS prefix is
    // evaluated from many threads (e.g. async project/code model updates).
    // Serialize all engine access; recursive to allow nested %{JS:...}.
    QRecursiveMutex m_mutex;
    JsEngineWatchdog m_watchdog;
};

} // namespace Internal

void JsExpander::registerGlobalObject(const QString &name, const ObjectFactory &factory)
{
    globalJsExtensions->insert({name, factory});
    if (globalExpander)
        globalExpander->registerObject(name, factory());
}

void JsExpander::registerObject(const QString &name, QObject *obj)
{
    QMutexLocker locker(&d->m_mutex);
    QJSValue jsObj = d->m_engine.newQObject(obj);
    d->m_engine.globalObject().setProperty(name, jsObj);
}

QString JsExpander::evaluate(const QString &expression, QString *errorMessage)
{
    QMutexLocker locker(&d->m_mutex);
    d->m_watchdog.watch(&d->m_engine);
    QJSValue value = d->m_engine.evaluate(expression);
    d->m_watchdog.unwatch();
    if (value.isError()) {
        const QString msg = Tr::tr("Error in \"%1\": %2").arg(expression, value.toString());
        if (errorMessage)
            *errorMessage = msg;
        return QString();
    }
    // Try to convert to bool, be that an int or whatever.
    if (value.isBool())
        return value.toString();
    if (value.isNumber())
        return QString::number(value.toNumber());
    if (value.isString())
        return value.toString();
    QString msg = Tr::tr("Cannot convert result of \"%1\" to string.").arg(expression);
    if (errorMessage)
        *errorMessage = msg;
    return QString();
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
    registerGlobalObject<Internal::UtilsJsExtension>("Util");
    globalExpander->registerForExpander(Utils::globalMacroExpander());
    return globalExpander;
}

JsExpander::JsExpander()
{
    d = new Internal::JsExpanderPrivate;
    for (const std::pair<const QString, ObjectFactory> &obj : *globalJsExtensions)
        registerObject(obj.first, obj.second());
}

JsExpander::~JsExpander()
{
    delete d;
    d = nullptr;
}

} // namespace Core


#ifdef WITH_TESTS

#include <utils/algorithm.h>

#include <QMutex>
#include <QTest>

namespace Core::Internal {

namespace {

// Records what qWarning() and friends emit while it is alive, so that a test can
// assert that a code path stays quiet. Messages arrive from any thread.
class MessageLog
{
public:
    MessageLog() { s_instance = this; m_previous = qInstallMessageHandler(&handler); }
    ~MessageLog() { qInstallMessageHandler(m_previous); s_instance = nullptr; }

    QStringList messages() const
    {
        QMutexLocker locker(&m_mutex);
        return m_messages;
    }

private:
    static void handler(QtMsgType type, const QMessageLogContext &context, const QString &message)
    {
        if (!s_instance)
            return;
        {
            QMutexLocker locker(&s_instance->m_mutex);
            s_instance->m_messages.append(message);
        }
        if (s_instance->m_previous)
            s_instance->m_previous(type, context, message);
    }

    mutable QMutex m_mutex;
    QStringList m_messages;
    QtMessageHandler m_previous = nullptr;
    static MessageLog *s_instance;
};

MessageLog *MessageLog::s_instance = nullptr;

} // namespace

class JsExpanderTest final : public QObject
{
    Q_OBJECT

private slots:
    void testExpanderStaysQuietWhenDestroyed();
};

// A JsExpander is created, used and destroyed right away in several places, most
// notably by every wizard factory that the "New File"/"New Project" dialog
// filters. Its evaluation watchdog runs in a thread of its own, which must not
// leave a running timer behind for the expander's destructor to trip over.
void JsExpanderTest::testExpanderStaysQuietWhenDestroyed()
{
    MessageLog log;

    for (int i = 0; i < 5; ++i) {
        JsExpander expander;
        QString errorMessage;
        // The watchdog interrupts an endless script. Getting that error back is
        // what proves its thread and its timer are up before the expander goes.
        expander.evaluate("for (;;) {}", &errorMessage);
        QVERIFY(!errorMessage.isEmpty());
        QCOMPARE(expander.evaluate("1 + 1"), QString("2"));
    }

    const QStringList timerMessages = Utils::filtered(log.messages(), [](const QString &message) {
        return message.contains("Timers cannot be stopped");
    });
    QVERIFY2(timerMessages.isEmpty(), qPrintable(timerMessages.join('\n')));
}

QObject *createJsExpanderTest()
{
    return new JsExpanderTest;
}

} // namespace Core::Internal

#endif // WITH_TESTS

#include "jsexpander.moc"
