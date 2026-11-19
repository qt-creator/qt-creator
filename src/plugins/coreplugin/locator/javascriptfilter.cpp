// Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javascriptfilter.h"

#include "ilocatorfilter.h"

#include "../coreplugintr.h"
#include "../icore.h"

#include <utils/algorithm.h>

#include <quickjs.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QPair>
#include <QPointer>
#include <QTimer>
#include <QWaitCondition>

#include <atomic>
#include <chrono>

using namespace Core;
using namespace Core::Internal;
using namespace QtTaskTree;
using namespace Utils;

using namespace std::chrono;
using namespace std::chrono_literals;

static const char s_initData[] = R"(
    function abs(x) { return Math.abs(x); }
    function acos(x) { return Math.acos(x); }
    function asin(x) { return Math.asin(x); }
    function atan(x) { return Math.atan(x); }
    function atan2(x, y) { return Math.atan2(x, y); }
    function bin(x) { return '0b' + x.toString(2); }
    function ceil(x) { return Math.ceil(x); }
    function cos(x) { return Math.cos(x); }
    function exp(x) { return Math.exp(x); }
    function e() { return Math.E; }
    function floor(x) { return Math.floor(x); }
    function hex(x) { return '0x' + x.toString(16); }
    function log(x) { return Math.log(x); }
    function max() { return Math.max.apply(null, arguments); }
    function min() { return Math.min.apply(null, arguments); }
    function oct(x) { return '0' + x.toString(8); }
    function pi() { return Math.PI; }
    function pow(x, y) { return Math.pow(x, y); }
    function random() { return Math.random(); }
    function round(x) { return Math.round(x); }
    function sin(x) { return Math.sin(x); }
    function sqrt(x) { return Math.sqrt(x); }
    function tan(x) { return Math.tan(x); }
)";

// Minimal QuickJS wrapper replacing QJSEngine for the calculator filter. Only
// evaluates expressions and supports cross-thread interruption; the interrupt
// flag is polled by QuickJS's interrupt handler during execution.
class JsInterpreter
{
public:
    JsInterpreter()
        : m_runtime(JS_NewRuntime())
        , m_context(JS_NewContext(m_runtime))
    {
        JS_SetInterruptHandler(m_runtime, &JsInterpreter::interruptHandler, this);
        // Fits the smallest thread stack Creator runs on (512 KB on macOS).
        JS_SetMaxStackSize(m_runtime, 256 * 1024);
    }
    ~JsInterpreter()
    {
        JS_FreeContext(m_context);
        JS_FreeRuntime(m_runtime);
    }
    Q_DISABLE_COPY_MOVE(JsInterpreter)

    // Returns the string form of the result, or of the exception when the
    // evaluation threw; isError is set accordingly.
    QString evaluate(const QString &input, bool *isError)
    {
        const QByteArray utf8 = input.toUtf8();
        JS_UpdateStackTop(m_runtime);
        const JSValue value = JS_Eval(m_context, utf8.constData(), utf8.size(),
                                      "<input>", JS_EVAL_TYPE_GLOBAL);
        *isError = JS_IsException(value);
        const JSValue string = *isError ? JS_GetException(m_context) : JS_DupValue(m_context, value);
        const char *utf8Result = JS_ToCString(m_context, string);
        const QString result = QString::fromUtf8(utf8Result);
        JS_FreeCString(m_context, utf8Result);
        JS_FreeValue(m_context, string);
        JS_FreeValue(m_context, value);
        return result;
    }

    void setInterrupted(bool interrupted) { m_interrupted.store(interrupted); }
    bool isInterrupted() const { return m_interrupted.load(); }

private:
    static int interruptHandler(JSRuntime *, void *opaque)
    {
        return static_cast<JsInterpreter *>(opaque)->m_interrupted.load() ? 1 : 0;
    }

    JSRuntime *m_runtime = nullptr;
    JSContext *m_context = nullptr;
    std::atomic_bool m_interrupted = false;
};

enum class JavaScriptResult {
    FinishedWithSuccess,
    FinishedWithError,
    TimedOut,
    Canceled
};

class JavaScriptOutput
{
public:
    QString m_output;
    JavaScriptResult m_result = JavaScriptResult::Canceled;
};

using JavaScriptCallback = std::function<void(const JavaScriptOutput &)>;

class JavaScriptInput
{
public:
    bool m_reset = false; // Recreates the engine, re-inits it and continues the request queue
    QString m_input;
    JavaScriptCallback m_callback = {};
};

class JavaScriptThread : public QObject
{
    Q_OBJECT

public:
    // Called from the other thread, scheduled from the main thread through the queued
    // invocation.
    void run();

    // Called from main thread exclusively
    void cancel();
    // Called from main thread exclusively
    int addRequest(const JavaScriptInput &input);
    // Called from main thread exclusively
    void removeRequest(int id);

    // Called from the main thread exclusively, scheduled from the other thread through the queued
    // invocation when the new result is ready.
    void flush();

signals:
    void newOutput();

private:
    struct QueueItem {
        int m_id = 0;
        JavaScriptInput m_input;
        std::optional<JavaScriptOutput> m_output = {};
    };

    // Called from the main thread exclusively
    QList<QueueItem> takeOutputQueue() {
        QMutexLocker locker(&m_mutex);
        return std::exchange(m_outputQueue, {});
    }

    int m_maxId = 0;
    std::unique_ptr<JsInterpreter> m_engine;

    mutable QMutex m_mutex;
    QWaitCondition m_waitCondition;
    bool m_canceled = false;
    QList<QueueItem> m_inputQueue;
    std::optional<QueueItem> m_currentItem;
    QList<QueueItem> m_outputQueue;
};

void JavaScriptThread::run()
{
    const auto evaluate = [this](const QString &input) {
        bool isError = false;
        const QString output = m_engine->evaluate(input, &isError);
        if (m_engine->isInterrupted()) {
            return JavaScriptOutput{Tr::tr("The evaluation was interrupted."),
                                    JavaScriptResult::Canceled};
        }
        return JavaScriptOutput{output,
                                isError ? JavaScriptResult::FinishedWithError
                                        : JavaScriptResult::FinishedWithSuccess};
    };
    const auto reset = [evaluate] {
        JavaScriptOutput output = evaluate(s_initData);
        output.m_output = output.m_result == JavaScriptResult::FinishedWithSuccess
                        ? Tr::tr("Engine reinitialized properly.")
                        : Tr::tr("Engine did not reinitialize properly.");
        return output;
    };

    {
        QMutexLocker locker(&m_mutex);
        if (m_canceled)
            return;
        m_engine.reset(new JsInterpreter);
    }

    // TODO: consider placing a reset request as the first input instead
    const JavaScriptOutput output = reset();
    QTC_ASSERT(output.m_result == JavaScriptResult::FinishedWithSuccess,
               qWarning() << output.m_output);

    QueueItem currentItem;
    while (true) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_canceled)
                return;
            if (m_currentItem) {
                QTC_CHECK(m_currentItem->m_id == currentItem.m_id);
                m_outputQueue.append(currentItem);
                m_currentItem = {};
                emit newOutput();
            }
            while (m_inputQueue.isEmpty()) {
                m_waitCondition.wait(&m_mutex);
                if (m_canceled)
                    return;
            }
            m_currentItem = currentItem = m_inputQueue.takeFirst();
            if (currentItem.m_input.m_reset)
                m_engine.reset(new JsInterpreter);
            m_engine->setInterrupted(false);
        }
        const JavaScriptInput &input = currentItem.m_input;
        if (input.m_reset) {
            currentItem.m_output = reset();
            QTC_ASSERT(currentItem.m_output->m_result == JavaScriptResult::FinishedWithSuccess,
                       qWarning() << currentItem.m_output->m_output);
            continue;
        }
        currentItem.m_output = evaluate(input.m_input);
    }
}

void JavaScriptThread::cancel()
{
    QMutexLocker locker(&m_mutex);
    m_canceled = true;
    if (m_engine) // we may be canceling before the run() started
        m_engine->setInterrupted(true);
    m_waitCondition.wakeOne();
}

int JavaScriptThread::addRequest(const JavaScriptInput &input)
{
    QMutexLocker locker(&m_mutex);
    if (input.m_reset) {
        if (m_currentItem) {
            m_outputQueue += *m_currentItem;
            m_engine->setInterrupted(true);
        }
        m_outputQueue += m_inputQueue;
        m_currentItem = {};
        m_inputQueue.clear();
        for (int i = 0; i < m_outputQueue.size(); ++i)
            m_outputQueue[i].m_output = {{}, JavaScriptResult::Canceled};
        QMetaObject::invokeMethod(this, &JavaScriptThread::newOutput, Qt::QueuedConnection);
    }
    m_inputQueue.append({++m_maxId, input});
    m_waitCondition.wakeOne();
    return m_maxId;
}

void JavaScriptThread::removeRequest(int id)
{
    QMutexLocker locker(&m_mutex);
    if (m_currentItem && m_currentItem->m_id == id) {
        m_currentItem = {};
        m_engine->setInterrupted(true);
        m_waitCondition.wakeOne();
        return;
    }
    const auto predicate = [id](const QueueItem &item) { return item.m_id == id; };
    if (Utils::eraseOne(m_inputQueue, predicate))
        return;
    Utils::eraseOne(m_outputQueue, predicate);
}

void JavaScriptThread::flush()
{
    const QList<QueueItem> outputQueue = takeOutputQueue();
    for (const QueueItem &item : outputQueue) {
        if (item.m_input.m_callback)
            item.m_input.m_callback(*item.m_output);
    }
}

class JavaScriptEngine : public QObject
{
    Q_OBJECT

public:
    JavaScriptEngine() : m_javaScriptThread(new JavaScriptThread) {
        connect(m_javaScriptThread, &JavaScriptThread::newOutput, this, [this] {
            m_javaScriptThread->flush();
        });
        m_javaScriptThread->moveToThread(&m_thread);
        QObject::connect(&m_thread, &QThread::finished, m_javaScriptThread, &QObject::deleteLater);
        m_thread.start();
        QMetaObject::invokeMethod(m_javaScriptThread, &JavaScriptThread::run);
    }
    ~JavaScriptEngine() {
        m_javaScriptThread->cancel();
        m_thread.quit();
        m_thread.wait();
    }

    int addRequest(const JavaScriptInput &input) { return m_javaScriptThread->addRequest(input); }
    void removeRequest(int id) { m_javaScriptThread->removeRequest(id); }

private:
    QThread m_thread;
    JavaScriptThread *m_javaScriptThread = nullptr;
};

class JavaScriptRequest : public QObject
{
    Q_OBJECT

public:
    virtual ~JavaScriptRequest()
    {
        if (m_engine && m_id) // In order to not to invoke a response callback anymore
            m_engine->removeRequest(*m_id);
    }

    void setEngine(JavaScriptEngine *engine) {
        QTC_ASSERT(!isRunning(), return);
        m_engine = engine;
    }
    void setReset(bool reset) {
        QTC_ASSERT(!isRunning(), return);
        m_input.m_reset = reset; // Reply: "Engine has been reset"?
    }
    void setEvaluateData(const QString &input) {
        QTC_ASSERT(!isRunning(), return);
        m_input.m_input = input;
    }
    void setTimeout(milliseconds timeout) {
        QTC_ASSERT(!isRunning(), return);
        m_timeout = timeout;
    }

    void start() {
        QTC_ASSERT(!isRunning(), return);
        QTC_ASSERT(m_engine, return);

        JavaScriptInput input = m_input;
        input.m_callback = [this](const JavaScriptOutput &output) {
            m_timer.reset();
            m_output = output;
            m_id = {};
            emit done(toDoneResult(output.m_result == JavaScriptResult::FinishedWithSuccess));
        };
        m_id = m_engine->addRequest(input);
        if (m_timeout > 0ms) {
            m_timer.reset(new QTimer);
            m_timer->setSingleShot(true);
            m_timer->setInterval(m_timeout);
            connect(m_timer.get(), &QTimer::timeout, this, [this] {
                if (m_engine && m_id)
                    m_engine->removeRequest(*m_id);
                m_timer.release()->deleteLater();
                m_id = {};
                m_output = {Tr::tr("Engine aborted after timeout."), JavaScriptResult::Canceled};
                emit done(DoneResult::Error);
            });
            m_timer->start();
        }
    }

    bool isRunning() const { return m_id.has_value(); }
    JavaScriptOutput output() const { return m_output; }

signals:
    void done(DoneResult result);

private:
    QPointer<JavaScriptEngine> m_engine;
    JavaScriptInput m_input;
    milliseconds m_timeout = 1000ms;

    std::unique_ptr<QTimer> m_timer;

    std::optional<int> m_id;
    JavaScriptOutput m_output;
};

using JavaScriptRequestTask = QCustomTask<JavaScriptRequest>;

namespace Core::Internal {

class JavaScriptFilter final : public ILocatorFilter
{
public:
    JavaScriptFilter();
    ~JavaScriptFilter();

private:
    LocatorMatcherTasks matchers() final;
    std::unique_ptr<JavaScriptEngine> m_javaScriptEngine;
};

void setupJavaScriptFilter()
{
    static JavaScriptFilter theJavaScriptFilter;
}

JavaScriptFilter::JavaScriptFilter()
{
    setId("JavaScriptFilter");
    setDisplayName(Tr::tr("Evaluate JavaScript"));
    setDescription(Tr::tr("Evaluates arbitrary JavaScript expressions and copies the result."));
    setDefaultShortcutString("=");
    connect(ICore::instance(), &ICore::coreAboutToClose, this, [this] {
        m_javaScriptEngine.reset();
    });
}

JavaScriptFilter::~JavaScriptFilter() = default;

LocatorMatcherTasks JavaScriptFilter::matchers()
{
    if (!m_javaScriptEngine)
        m_javaScriptEngine.reset(new JavaScriptEngine);
    QPointer<JavaScriptEngine> engine = m_javaScriptEngine.get();

    const auto onSetup = [engine] {
        const LocatorStorage &storage = *LocatorStorage::storage();
        if (!engine)
            return SetupResult::StopWithError;
        if (storage.input().trimmed().isEmpty()) {
            LocatorFilterEntry entry;
            entry.displayName = Tr::tr("Reset Engine");
            entry.completer = [] { return AcceptResult(); }; // do not complete
            entry.acceptor = [engine] {
                if (engine) {
                    JavaScriptInput request;
                    request.m_reset = true;
                    engine->addRequest(request); // TODO: timeout not handled
                }
                return AcceptResult();
            };
            storage.reportOutput({entry});
            return SetupResult::StopWithSuccess;
        }
        return SetupResult::Continue;
    };

    const auto onJavaScriptSetup = [engine](JavaScriptRequest &request) {
        request.setEngine(engine);
        request.setEvaluateData(LocatorStorage::storage()->input());
    };
    const auto onJavaScriptDone = [](const JavaScriptRequest &request, DoneWith result) {
        const LocatorStorage &storage = *LocatorStorage::storage();
        if (result != DoneWith::Success) {
            LocatorFilterEntry entry;
            entry.displayName = request.output().m_output;
            storage.reportOutput({entry});
            return;
        }
        const auto acceptor = [](const QString &clipboardContents) {
            return [clipboardContents] {
                QGuiApplication::clipboard()->setText(clipboardContents);
                return AcceptResult();
            };
        };
        const QString input = storage.input();
        const QString output = request.output().m_output;
        const QString expression = input + " = " + output;

        LocatorFilterEntry entry;
        entry.displayName = expression;
        entry.completer = [] { return AcceptResult(); }; // do not complete

        LocatorFilterEntry copyResultEntry;
        copyResultEntry.displayName = Tr::tr("Copy to clipboard: %1").arg(output);
        copyResultEntry.completer = [] { return AcceptResult(); }; // do not complete
        copyResultEntry.acceptor = acceptor(output);

        LocatorFilterEntry copyExpressionEntry;
        copyExpressionEntry.displayName = Tr::tr("Copy to clipboard: %1").arg(expression);
        copyExpressionEntry.completer = [] { return AcceptResult(); }; // do not complete
        copyExpressionEntry.acceptor = acceptor(expression);

        storage.reportOutput({entry, copyResultEntry, copyExpressionEntry});
    };

    const Group root {
        onGroupSetup(onSetup),
        JavaScriptRequestTask(onJavaScriptSetup, onJavaScriptDone)
    };

    return {root};
}

} // namespace Core::Internal

#include "javascriptfilter.moc"
