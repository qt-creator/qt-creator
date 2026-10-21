// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerfindingsmodel.h"

#include "profilertr.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilersettings.h"

#include <tracing/timelineformatdata.h>
#include <utils/qtcassert.h>

#include <QHash>
#include <QSet>
#include <QStack>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>

using namespace QmlDebug;

namespace Profiler::Internal {

// Ranges nest, so a RangeEnd belongs to the RangeStart that opened the same type. A trace
// can begin inside a range, so the match is not necessarily on top of the stack: opens left
// above it are dropped, the way QmlProfilerStatisticsModel does.
class RangeStack
{
public:
    struct Frame
    {
        int typeIndex = -1;
        RangeType rangeType = UndefinedRangeType;
        qint64 startTime = 0;
    };

    void push(int typeIndex, RangeType rangeType, qint64 startTime)
    {
        m_frames.push({typeIndex, rangeType, startTime});
    }

    std::optional<Frame> pop(int typeIndex)
    {
        for (int i = m_frames.size() - 1; i >= 0; --i) {
            if (m_frames.at(i).typeIndex != typeIndex)
                continue;
            const Frame frame = m_frames.at(i);
            m_frames.resize(i);
            return frame;
        }
        return {};
    }

    bool isEmpty() const { return m_frames.isEmpty(); }
    const Frame &top() const { return m_frames.top(); }
    void clear() { m_frames.clear(); }

private:
    QStack<Frame> m_frames;
};

// Sums the time spent in ranges of one type, per event type. The rules that only need
// "how much, how often" differ in nothing else but the range type they watch.
class RangeTotals
{
public:
    struct Total
    {
        qint64 totalNs = 0;
        int count = 0;
    };

    void loadEvent(const QmlEvent &event, RangeType rangeType)
    {
        const int typeIndex = event.typeIndex();
        switch (event.rangeStage()) {
        case RangeStart:
            m_stack.push(typeIndex, rangeType, event.timestamp());
            break;
        case RangeEnd: {
            const std::optional<RangeStack::Frame> frame = m_stack.pop(typeIndex);
            if (!frame)
                break;

            Total &total = m_totals[typeIndex];
            total.totalNs += event.timestamp() - frame->startTime;
            ++total.count;
            break;
        }
        default:
            break;
        }
    }

    QHash<int, Total>::const_iterator begin() const { return m_totals.constBegin(); }
    QHash<int, Total>::const_iterator end() const { return m_totals.constEnd(); }

    void clear()
    {
        m_totals.clear();
        m_stack.clear();
    }

private:
    QHash<int, Total> m_totals;
    RangeStack m_stack;
};

// Picks one of the threads that report frames. The GUI and the render thread both report
// a frame event for the same rendering, so following the one that reported first keeps
// the gap between consecutive events equal to a frame duration and their number equal to
// the number of frames; interleaving the two halves the one and doubles the other.
class FrameThread
{
public:
    bool accepts(const QmlEvent &event)
    {
        const int thread = event.number<qint32>(2);
        if (m_thread == -1)
            m_thread = thread;
        return thread == m_thread;
    }

    void clear() { m_thread = -1; }

private:
    int m_thread = -1;
};

// Reports QML files whose first-use compilation dominates startup. Ahead-of-time caching
// does not remove this: what is left in a Compiling range is loading the compilation unit
// and resolving types and imports, which scales with file size and import surface.
class FirstUseCompileRule final : public FindingRule
{
public:
    explicit FirstUseCompileRule(qint64 thresholdNs) : m_thresholdNs(thresholdNs) {}

    quint64 features() const override { return 1ULL << ProfileCompiling; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.rangeType() == Compiling)
            m_totals.loadEvent(event, Compiling);
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_totals.begin(), end = m_totals.end(); it != end; ++it) {
            if (it->totalNs < m_thresholdNs)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);
            const QmlEventType &type = manager->eventType(typeIndex);

            Finding finding;
            finding.ruleId = "first-use-compile";
            finding.severity = it->totalNs >= 4 * m_thresholdNs ? Finding::Warning : Finding::Info;
            finding.location = type.location();
            finding.typeIndex = typeIndex;
            finding.costNs = it->totalNs;
            finding.occurrences = it->count;
            finding.what = Tr::tr("Compiling this file costs %1 ms on first use.")
                               .arg(it->totalNs / 1000000);
            finding.why = Tr::tr("Even with ahead-of-time QML caching, first use still loads the "
                                 "compilation unit and resolves the file's types and imports. The "
                                 "cost grows with file size and with the number of imports pulled "
                                 "in before the file can be instantiated.");
            finding.suggestion = Tr::tr("Split the file into smaller components, each compiled and "
                                        "loaded on its own, and move imports that only a lazily "
                                        "loaded part needs into that part.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override { m_totals.clear(); }

private:
    RangeTotals m_totals;
    const qint64 m_thresholdNs;
};

// Reports images the engine failed to load. Each failure is a defect in the application,
// and because a failing source is usually requested again, it also burns repeated work.
class PixmapLoadErrorRule final : public FindingRule
{
public:
    quint64 features() const override { return 1ULL << ProfilePixmapCache; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() != PixmapCacheEvent || type.detailType() != PixmapLoadingError)
            return;
        ++m_errors[event.typeIndex()];
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_errors.constBegin(), end = m_errors.constEnd(); it != end; ++it) {
            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);
            const QmlEventType &type = manager->eventType(typeIndex);

            Finding finding;
            finding.ruleId = "pixmap-load-error";
            finding.severity = Finding::Critical;
            finding.location = type.location();
            finding.typeIndex = typeIndex;
            finding.occurrences = it.value();
            finding.what = Tr::tr("Image failed to load %n time(s).", nullptr, it.value());
            finding.why = Tr::tr("The engine could not load this source, so nothing is painted for "
                                 "it. Repeated attempts mean the failure is retried rather than "
                                 "cached, so each one costs another request.");
            finding.suggestion = Tr::tr("Check that the source resolves: a missing resource path, a "
                                        "typo in the URL, or an image provider that rejects the "
                                        "request are the usual causes.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override { m_errors.clear(); }

private:
    QHash<int, int> m_errors;
};

// Follows the engine building component instances. The trace has no event for "an instance
// was created", only Creating ranges, one per object, and they come in two phases: the
// object is created, nested inside its parent, and once the whole tree exists and its
// bindings have run, every object that implements QQmlParserStatus is completed, children
// before parents, in a range of the same type. So an Item shows up twice, a QtObject once,
// and the composite type wrapping another file's root not at all on completion. Telling the
// two apart is what makes instance counts right.
//
// An instance is a Creating range that is not nested in another object's creation, and
// not an object of an earlier instance that is only created when first needed. What runs
// directly next to it until its root is completed - its bindings, completions and
// Component.onCompleted handlers - is part of building it.
class InstanceTracker
{
public:
    // What made the engine build an instance.
    struct Context
    {
        enum Kind {
            Unknown, // No QML or JavaScript was running: C++ built it.
            Owner,   // An object built it while completing, or owns the component it is from.
            Script   // JavaScript created it.
        };

        Kind kind = Unknown;
        int typeIndex = -1; // The owning object's Creating type, or the script's type.

        friend bool operator==(const Context &a, const Context &b)
        {
            return a.kind == b.kind && a.typeIndex == b.typeIndex;
        }
        friend size_t qHash(const Context &context, size_t seed = 0)
        {
            return qHashMulti(seed, int(context.kind), context.typeIndex);
        }
    };

    // Where the time to build an instance goes, by event type: an object's own time without
    // what nests inside it, the full time of a binding or handler, and instances of other
    // components that were built inside this one.
    struct Part
    {
        qint64 costNs = 0;
        int count = 0;
        bool nestedInstance = false;
        bool createdLater = false;
    };

    struct Instance
    {
        int rootType = -1;
        qint64 costNs = 0;
        Context context;
        // The outermost handler or function that was running when the instance was built,
        // and which call of it, if the instance was built by JavaScript.
        int callType = -1;
        int callId = -1;
        // An object of another component created when that one first needed it, like the
        // animation of a Behavior or the changes of a State: the root of that component.
        int partOf = -1;
        QHash<int, Part> parts;

        QList<int> pendingCompletions;
        QHash<QString, int> firstLines; // Where the instance's objects begin, per file.
        bool completed = false;
    };

    using InstanceHandler = std::function<void(const Instance &)>;

    explicit InstanceTracker(const InstanceHandler &handler) : m_handler(handler) {}

    static quint64 features()
    {
        return (1ULL << ProfileCreating) | (1ULL << ProfileBinding)
               | (1ULL << ProfileHandlingSignal) | (1ULL << ProfileJavaScript);
    }

    void loadEvent(const QmlEvent &event, const QmlEventType &type)
    {
        const RangeType rangeType = type.rangeType();
        if (rangeType != Creating && rangeType != Binding && rangeType != HandlingSignal
            && rangeType != Javascript) {
            return;
        }

        switch (event.rangeStage()) {
        case RangeStart:
            start(event, type);
            break;
        case RangeEnd:
            end(event);
            break;
        default:
            break;
        }
    }

    // The name a binding or signal handler gives in the trace: the property or the signal.
    // The engine reports it on the JavaScript range nested in it, as "expression for x".
    QString name(int typeIndex) const { return m_names.value(typeIndex); }

    void finish()
    {
        while (!m_frames.isEmpty()) {
            Frame frame = m_frames.pop();
            finishInstance(frame);
        }
        finishInstance(m_root);
    }

    void clear()
    {
        m_frames.clear();
        m_root = {};
        m_instances.clear();
        m_componentOwners.clear();
        m_componentLines.clear();
        m_fileRoots.clear();
        m_owners.clear();
        m_names.clear();
        m_nextInstance = 0;
        m_nextCall = 0;
    }

private:
    struct Frame
    {
        enum Kind { Creation, Completion, Work, Other };

        int typeIndex = -1;
        RangeType rangeType = UndefinedRangeType;
        Kind kind = Other;
        qint64 startTime = 0;
        qint64 childrenNs = 0;
        int instance = -1;     // The instance this frame's time belongs to.
        int openInstance = -1; // The instance being built directly inside this frame.
        int call = -1;
        bool hasChild = false;
        bool maybeCompleted = false; // A handler that may be the instance's onCompleted.
        QString filename;
        int line = 0;
    };

    Frame &top() { return m_frames.isEmpty() ? m_root : m_frames.top(); }

    void start(const QmlEvent &event, const QmlEventType &type)
    {
        Frame &parent = top();
        Frame frame;
        frame.typeIndex = event.typeIndex();
        frame.rangeType = type.rangeType();
        frame.startTime = event.timestamp();
        const QmlEventLocation location = type.location();
        frame.filename = location.filename();
        frame.line = location.line();
        frame.call = m_nextCall++;

        if (frame.rangeType == Creating)
            startCreating(parent, frame, type);
        else
            startWork(parent, frame, type);

        m_frames.push(frame);
    }

    void startCreating(Frame &parent, Frame &frame, const QmlEventType &type)
    {
        // A component written inline, like a delegate, is created as a value of the
        // object whose property it is. Instances of it are that object's.
        const bool isComponent = type.data() == QLatin1String("<component>");

        if (parent.kind == Frame::Creation) {
            frame.instance = parent.instance;
            if (isComponent) {
                m_componentOwners.insert(type.location(), parent.typeIndex);
                m_componentLines[frame.filename].append(frame.line);
                return;
            }

            auto it = m_instances.find(parent.instance);
            if (it == m_instances.end())
                return;
            Instance &instance = *it;
            // A composite type is created by creating the root of its file inside it, and
            // completing that root is all the completing it gets.
            if (!parent.hasChild && parent.filename != frame.filename
                && !instance.pendingCompletions.isEmpty()
                && instance.pendingCompletions.last() == parent.typeIndex) {
                instance.pendingCompletions.removeLast();
            }
            parent.hasChild = true;
            frame.kind = Frame::Creation;
            addObject(instance, frame);
            return;
        }

        if (auto it = m_instances.find(parent.openInstance); it != m_instances.end()) {
            Instance &instance = *it;
            const int pending = instance.pendingCompletions.lastIndexOf(frame.typeIndex);
            if (pending >= 0) {
                // Completions run in the reverse order of creation. Objects created after
                // this one that have not been completed yet never will be.
                instance.pendingCompletions.resize(pending);
                frame.kind = Frame::Completion;
                frame.instance = parent.openInstance;
                return;
            }
        }

        if (isComponent) {
            frame.instance = parent.instance;
            return;
        }

        // Some objects are created only when they are first needed, after the rest of their
        // component: a Behavior's animation while the instance is finalized, a State's
        // changes when it first applies.
        const int laterPartOf = parent.openInstance >= 0 ? parent.openInstance : parent.instance;
        if (auto it = m_instances.find(laterPartOf);
            it != m_instances.end() && it->rootType != frame.typeIndex
            && isLaterPart(it->firstLines, frame)) {
            frame.kind = Frame::Creation;
            frame.instance = laterPartOf;
            addObject(*it, frame);
            return;
        }

        finishInstance(parent);

        const int id = m_nextInstance++;
        Instance &instance = m_instances[id];
        instance.rootType = frame.typeIndex;
        instance.context = context(parent, type);
        const std::optional<std::pair<int, int>> call = outermostCall();
        if (call) {
            instance.callType = call->first;
            instance.callId = call->second;
        }
        if (parent.kind != Frame::Completion) {
            const auto root = m_fileRoots.constFind(frame.filename);
            if (root != m_fileRoots.constEnd()
                && isLaterPart({{frame.filename, root->first}}, frame)) {
                instance.partOf = root->second;
            }
        }
        parent.openInstance = id;
        frame.kind = Frame::Creation;
        frame.instance = id;
        addObject(instance, frame);
    }

    // Whether an object lies inside a component, given where the component's objects begin,
    // and is not the root of an inline component of its own.
    bool isLaterPart(const QHash<QString, int> &firstLines, const Frame &frame) const
    {
        const auto first = firstLines.constFind(frame.filename);
        if (first == firstLines.constEnd() || frame.line <= *first)
            return false;
        const QList<int> components = m_componentLines.value(frame.filename);
        return std::none_of(components.begin(), components.end(), [&](int line) {
            return line > *first && line <= frame.line;
        });
    }

    void startWork(Frame &parent, Frame &frame, const QmlEventType &type)
    {
        frame.kind = Frame::Work;
        frame.instance = parent.instance;
        if (parent.openInstance >= 0) {
            // While an instance is finalized, the engine runs its bindings, completes its
            // objects and then calls its Component.onCompleted handlers, each in a range of
            // its own next to the instance's. A function called there, or a binding once the
            // instance is complete, is the caller going on. A handler is onCompleted only if
            // the JavaScript range nested in it says so.
            const auto open = m_instances.constFind(parent.openInstance);
            const bool completed = open == m_instances.constEnd() || open->completed;
            if (frame.rangeType == Javascript || (frame.rangeType == Binding && completed)) {
                finishInstance(parent);
            } else {
                frame.instance = parent.openInstance;
                frame.maybeCompleted = frame.rangeType == HandlingSignal;
            }
        }

        static const QLatin1String expressionFor("expression for ");
        if (frame.rangeType != Javascript || parent.kind != Frame::Work
            || parent.rangeType == Javascript) {
            return;
        }

        // This runs for most events of a trace, so the name is only copied once per type.
        const QString data = type.data();
        const QStringView name = data.startsWith(expressionFor)
                                     ? QStringView(data).sliced(expressionFor.size())
                                     : QStringView();
        if (!name.isEmpty() && !m_names.contains(parent.typeIndex))
            m_names.insert(parent.typeIndex, name.toString());

        if (parent.maybeCompleted) {
            parent.maybeCompleted = false;
            if (name != QLatin1String("onCompleted")) {
                Frame &grandParent = m_frames.size() > 1 ? m_frames[m_frames.size() - 2]
                                                         : m_root;
                parent.instance = grandParent.instance;
                frame.instance = parent.instance;
                finishInstance(grandParent);
            }
        }
    }

    void end(const QmlEvent &event)
    {
        std::optional<Frame> found;
        while (!m_frames.isEmpty()) {
            Frame frame = m_frames.pop();
            finishInstance(frame);
            if (frame.typeIndex == event.typeIndex()) {
                found = frame;
                break;
            }
        }
        // A range that started before the trace did: what was left above it is dropped,
        // the way RangeStack does.
        if (!found)
            return;

        const Frame &frame = *found;
        const qint64 durationNs = event.timestamp() - frame.startTime;
        Frame &parent = top();
        parent.childrenNs += durationNs;

        auto it = m_instances.find(frame.instance);
        if (it == m_instances.end())
            return;
        Instance &instance = *it;

        switch (frame.kind) {
        case Frame::Creation:
        case Frame::Completion: {
            Part &part = instance.parts[frame.typeIndex];
            part.costNs += durationNs - frame.childrenNs;
            if (frame.kind == Frame::Creation)
                ++part.count;
            else if (instance.pendingCompletions.isEmpty())
                instance.completed = true;
            break;
        }
        case Frame::Work:
            // Only the outermost binding or handler is named. What nests in it is part of
            // its cost.
            if ((parent.kind != Frame::Work || parent.instance != frame.instance)
                && belongsTo(instance, frame)) {
                Part &part = instance.parts[frame.typeIndex];
                part.costNs += durationNs;
                ++part.count;
            }
            break;
        case Frame::Other:
            break;
        }

        if (parent.openInstance == frame.instance)
            instance.costNs += durationNs;
    }

    // While an instance is built, the engine may evaluate bindings elsewhere that react to
    // it. Those are not the instance's to make cheaper.
    static bool belongsTo(const Instance &instance, const Frame &frame)
    {
        const auto it = instance.firstLines.constFind(frame.filename);
        return it != instance.firstLines.constEnd() && frame.line >= *it;
    }

    static void addObject(Instance &instance, const Frame &frame)
    {
        instance.pendingCompletions.append(frame.typeIndex);
        auto it = instance.firstLines.find(frame.filename);
        if (it == instance.firstLines.end())
            instance.firstLines.insert(frame.filename, frame.line);
        else if (frame.line < *it)
            *it = frame.line;
    }

    Context context(const Frame &parent, const QmlEventType &type)
    {
        // An object that builds instances while it is completed is a view or a Loader
        // filling itself. Remember it: later instances of the same component are built
        // when nothing in the trace says who asked for them, like a view scrolling.
        if (parent.kind == Frame::Completion) {
            m_owners.insert(type.location(), parent.typeIndex);
            return {Context::Owner, parent.typeIndex};
        }

        int owner = m_componentOwners.value(type.location(), -1);
        if (owner < 0)
            owner = m_owners.value(type.location(), -1);
        if (owner >= 0)
            return {Context::Owner, owner};

        int script = -1;
        for (int i = m_frames.size() - 1; i >= 0; --i) {
            const Frame &frame = m_frames.at(i);
            if (frame.kind != Frame::Work)
                continue;
            if (frame.rangeType == Javascript)
                return {Context::Script, frame.typeIndex};
            if (script < 0)
                script = frame.typeIndex;
        }
        if (script >= 0)
            return {Context::Script, script};
        return {};
    }

    // The handler or function that the user waits for while the instance is built: the
    // outermost one of the calls directly above it. Calls made while building another
    // instance are that instance's.
    std::optional<std::pair<int, int>> outermostCall() const
    {
        std::optional<std::pair<int, int>> call;
        for (int i = m_frames.size() - 1; i >= 0; --i) {
            const Frame &frame = m_frames.at(i);
            if (frame.kind != Frame::Work || frame.instance >= 0)
                break;
            if (frame.rangeType != Binding)
                call = std::make_pair(frame.typeIndex, frame.call);
        }
        return call;
    }

    void finishInstance(Frame &frame)
    {
        if (frame.openInstance < 0)
            return;

        const Instance instance = m_instances.take(frame.openInstance);
        frame.openInstance = -1;
        m_handler(instance);

        if (instance.partOf < 0) {
            for (auto it = instance.firstLines.constBegin(), end = instance.firstLines.constEnd();
                 it != end; ++it) {
                const auto root = m_fileRoots.constFind(it.key());
                if (root == m_fileRoots.constEnd() || it.value() < root->first)
                    m_fileRoots.insert(it.key(), {it.value(), instance.rootType});
            }
        }

        // An instance built while another one was, like the delegates of a view inside a
        // delegate, is part of what building the outer one costs.
        auto outer = m_instances.find(frame.instance);
        if (outer != m_instances.end()) {
            Part &part = outer->parts[instance.rootType];
            part.costNs += instance.costNs;
            ++part.count;
            part.nestedInstance = true;
        }
    }

    const InstanceHandler m_handler;
    QStack<Frame> m_frames;
    Frame m_root;
    QHash<int, Instance> m_instances;
    QHash<QmlEventLocation, int> m_componentOwners;
    QHash<QString, QList<int>> m_componentLines;
    // Where the first object of a file was seen, and the root of the instance it was in.
    QHash<QString, std::pair<int, int>> m_fileRoots;
    QHash<QmlEventLocation, int> m_owners;
    QHash<int, QString> m_names;
    int m_nextInstance = 0;
    int m_nextCall = 0;
};

// The type a Creating range reports carries the module, like "QtQuick/ListView".
static QString objectTypeName(const QmlEventType &type)
{
    const QString data = type.data();
    const QString name = data.mid(data.lastIndexOf(QLatin1Char('/')) + 1);
    return name.isEmpty() ? Tr::tr("object") : name;
}

static bool isHandlerName(const QString &name)
{
    return name.size() > 2 && name.startsWith(QLatin1String("on")) && name.at(2).isUpper();
}

// How a binding, a signal handler or a function is called in a sentence.
static QString scriptName(const QmlEventType &type, const QString &name)
{
    static const QLatin1String expressionFor("expression for ");
    QString data = type.data();
    if (data.startsWith(expressionFor))
        data = data.mid(expressionFor.size());

    switch (type.rangeType()) {
    case Binding:
        return name.isEmpty() ? Tr::tr("binding") : Tr::tr("%1 binding").arg(name);
    case HandlingSignal:
        return name.isEmpty() ? Tr::tr("signal handler") : Tr::tr("%1 handler").arg(name);
    case Javascript:
        if (type.data().startsWith(expressionFor)) {
            return isHandlerName(data) ? Tr::tr("%1 handler").arg(data)
                                       : Tr::tr("%1 binding").arg(data);
        }
        return data.isEmpty() ? Tr::tr("anonymous function") : data + QLatin1String("()");
    default:
        return objectTypeName(type);
    }
}

// The views that keep a pool of delegates when reuseItems is set. TableView and TreeView
// do so by default.
static bool isReusingView(const QString &typeName)
{
    return typeName == QLatin1String("ListView") || typeName == QLatin1String("GridView");
}

static bool reusesByDefault(const QString &typeName)
{
    return typeName == QLatin1String("TableView") || typeName == QLatin1String("TreeView")
           || typeName == QLatin1String("HorizontalHeaderView")
           || typeName == QLatin1String("VerticalHeaderView");
}

// The types that build one instance per model entry, all of them at once.
static bool buildsEveryEntry(const QString &typeName)
{
    return typeName == QLatin1String("Repeater") || typeName == QLatin1String("Instantiator");
}

// Tracking instances costs as much as all other rules together, so the rules that look at
// them share it: the first rule to subscribe feeds the events, and finishing is done once,
// for whichever rule needs the instances first.
class SharedInstanceTracker
{
public:
    SharedInstanceTracker()
        : m_tracker([this](const InstanceTracker::Instance &instance) {
            for (const InstanceTracker::InstanceHandler &handler : std::as_const(m_handlers))
                handler(instance);
        })
    {}

    void subscribe(const FindingRule *rule, const InstanceTracker::InstanceHandler &handler)
    {
        if (!m_feeder)
            m_feeder = rule;
        m_handlers.append(handler);
    }

    void loadEvent(const FindingRule *rule, const QmlEvent &event, const QmlEventType &type)
    {
        if (rule == m_feeder)
            m_tracker.loadEvent(event, type);
    }

    void finish()
    {
        if (!std::exchange(m_finished, true))
            m_tracker.finish();
    }

    void clear(const FindingRule *rule)
    {
        if (rule != m_feeder)
            return;
        m_tracker.clear();
        m_finished = false;
    }

    QString name(int typeIndex) const { return m_tracker.name(typeIndex); }

private:
    InstanceTracker m_tracker;
    QList<InstanceTracker::InstanceHandler> m_handlers;
    const FindingRule *m_feeder = nullptr;
    bool m_finished = false;
};

static InstanceTracker::Context dominantContext(
    const QHash<InstanceTracker::Context, int> &contexts)
{
    InstanceTracker::Context result;
    int most = 0;
    for (auto it = contexts.constBegin(), end = contexts.constEnd(); it != end; ++it) {
        if (it.value() > most) {
            most = it.value();
            result = it.key();
        }
    }
    return result;
}

static QmlEventLocation locationOf(const QmlProfilerModelManager *manager, int typeIndex)
{
    QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), return {});
    return manager->eventType(typeIndex).location();
}
// Reports handlers that build items while the caller waits. Assigning a Loader's source,
// or creating objects from a signal handler, runs the whole creation before the handler
// returns, and nothing can repaint until it does.
class SyncViewLoadRule final : public FindingRule
{
public:
    explicit SyncViewLoadRule(qint64 thresholdNs,
                              const std::shared_ptr<SharedInstanceTracker> &tracker
                              = std::make_shared<SharedInstanceTracker>())
        : m_tracker(tracker)
        , m_thresholdNs(thresholdNs)
    {
        m_tracker->subscribe(this, [this](const InstanceTracker::Instance &instance) {
            add(instance);
        });
    }

    quint64 features() const override { return InstanceTracker::features(); }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        m_tracker->loadEvent(this, event, type);
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        m_tracker->finish();

        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            if (it->totalNs < m_thresholdNs)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "sync-view-load";
            finding.severity = Finding::Warning;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.costNs = it->totalNs;
            finding.occurrences = it->calls;
            finding.what = Tr::tr("Building items inside this handler costs %1 over %n call(s).",
                                  nullptr, it->calls)
                               .arg(Timeline::formatTime(it->totalNs));
            finding.why = Tr::tr("The items are created while the handler runs, so the interface "
                                 "cannot repaint until the whole component is built. The longer "
                                 "the component, the longer the interface stands still.");

            QList<std::pair<int, Built>> built;
            for (auto b = it->built.constBegin(), bEnd = it->built.constEnd(); b != bEnd; ++b)
                built.append({b.key(), b.value()});
            std::sort(built.begin(), built.end(), [](const auto &a, const auto &b) {
                return a.second.costNs > b.second.costNs;
            });

            const InstanceTracker::Context context
                = built.isEmpty() ? InstanceTracker::Context()
                                  : dominantContext(built.first().second.contexts);
            finding.suggestion = suggestion(manager, context);

            for (const auto &[rootType, data] : built.mid(0, maxDetails)) {
                FindingDetail detail;
                detail.text = Tr::tr("Builds %1, %n time(s)", nullptr, data.count)
                                  .arg(objectTypeName(manager->eventType(rootType)));
                detail.location = locationOf(manager, rootType);
                detail.typeIndex = rootType;
                detail.costNs = data.costNs;
                detail.occurrences = data.count;
                finding.details.append(detail);
            }
            if (context.kind == InstanceTracker::Context::Script
                && context.typeIndex != typeIndex) {
                FindingDetail detail;
                detail.text = Tr::tr("Created in %1")
                                  .arg(scriptName(manager->eventType(context.typeIndex),
                                                  m_tracker->name(context.typeIndex)));
                detail.location = locationOf(manager, context.typeIndex);
                detail.typeIndex = context.typeIndex;
                finding.details.append(detail);
            }
            findings.append(finding);
        }
        return findings;
    }

    void clear() override
    {
        m_tracker->clear(this);
        m_data.clear();
    }

private:
    struct Built
    {
        qint64 costNs = 0;
        int count = 0;
        QHash<InstanceTracker::Context, int> contexts;
    };

    struct Data
    {
        qint64 totalNs = 0;
        int calls = 0;
        int lastCall = -1;
        QHash<int, Built> built;
    };

    void add(const InstanceTracker::Instance &instance)
    {
        if (instance.callType < 0)
            return;

        // Attribute the creation to the handler that waited for it: that is where the
        // load has to be made asynchronous.
        Data &data = m_data[instance.callType];
        data.totalNs += instance.costNs;
        if (instance.callId != data.lastCall) {
            data.lastCall = instance.callId;
            ++data.calls;
        }
        if (instance.partOf >= 0) {
            // Part of something built earlier, like the changes of a State the handler
            // applies: it costs the handler time, but it is not an item it builds.
            if (auto part = data.built.find(instance.partOf); part != data.built.end())
                part->costNs += instance.costNs;
            return;
        }
        Built &built = data.built[instance.rootType];
        built.costNs += instance.costNs;
        ++built.count;
        ++built.contexts[instance.context];
    }

    static QString suggestion(const QmlProfilerModelManager *manager,
                              const InstanceTracker::Context &context)
    {
        if (context.kind == InstanceTracker::Context::Owner) {
            const QmlEventType &owner = manager->eventType(context.typeIndex);
            const QString ownerName = objectTypeName(owner);
            if (ownerName == QLatin1String("Loader")) {
                return Tr::tr("Set asynchronous: true on the Loader at %1, so that it builds its "
                              "item over several frames, and show a placeholder while it is "
                              "loading.").arg(owner.displayName());
            }
            return Tr::tr("The %1 at %2 builds these items in response to what the handler "
                          "changes. Make that change in smaller steps, or move it out of the "
                          "handler, so that the interface can repaint in between.")
                .arg(ownerName, owner.displayName());
        }
        // A Loader whose source is set to a file and a component created from JavaScript
        // look the same in the trace.
        return Tr::tr("If the handler creates the items with createObject(), use "
                      "incubateObject() instead, so that they are built over several frames. If "
                      "it sets the source of a Loader, set asynchronous: true on that Loader. "
                      "Either way, show a placeholder until the items are ready.");
    }

    static constexpr int maxDetails = 3;

    QHash<int, Data> m_data;
    const std::shared_ptr<SharedInstanceTracker> m_tracker;
    const qint64 m_thresholdNs;
};

// Reports handlers that run at a steady interval. Whether that is wasted work depends on
// what the handler does when nothing is visible, which the trace cannot say - so this
// points at the pattern and leaves the judgement to the reader.
class PeriodicHandlerRule final : public FindingRule
{
public:
    PeriodicHandlerRule(int minCount, double maxRelativeDeviation)
        : m_minCount(minCount)
        , m_maxRelativeDeviation(maxRelativeDeviation)
    {}

    quint64 features() const override { return 1ULL << ProfileHandlingSignal; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.rangeType() != HandlingSignal || event.rangeStage() != RangeStart)
            return;

        Data &data = m_data[event.typeIndex()];
        if (data.count > 0) {
            // Keep running sums instead of every timestamp: a handler can fire thousands
            // of times in a trace.
            const qint64 interval = event.timestamp() - data.lastStart;
            data.intervalSum += interval;
            data.intervalSquareSum += double(interval) * double(interval);
            ++data.intervals;
        }
        data.lastStart = event.timestamp();
        ++data.count;
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            if (it->count < m_minCount || it->intervals < 2)
                continue;

            const double mean = double(it->intervalSum) / it->intervals;
            if (mean <= 0)
                continue;
            const double variance = it->intervalSquareSum / it->intervals - mean * mean;
            const double deviation = variance > 0 ? std::sqrt(variance) : 0;
            if (deviation / mean > m_maxRelativeDeviation)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "periodic-handler";
            finding.severity = Finding::Info;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.occurrences = it->count;
            finding.what = Tr::tr("Runs %n time(s), about every %1 ms.", nullptr, it->count)
                               .arg(qRound(mean / 1000000.0));
            finding.why = Tr::tr("A handler driven by a timer keeps running at that interval for "
                                 "as long as the timer does, whether or not anything comes of it.");
            finding.suggestion = Tr::tr("Check what this does when its target is not visible or "
                                        "has no data to show. If the answer is nothing, stop the "
                                        "timer under the same condition that hides the target.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override { m_data.clear(); }

private:
    struct Data {
        qint64 lastStart = 0;
        qint64 intervalSum = 0;
        double intervalSquareSum = 0;
        int intervals = 0;
        int count = 0;
    };

    QHash<int, Data> m_data;
    const int m_minCount;
    const double m_maxRelativeDeviation;
};

// Reports images decoded at a size the interface is unlikely to need. The cache holds the
// decoded image at the size it was loaded, whatever size it is drawn at.
class OversizedPixmapRule final : public FindingRule
{
public:
    explicit OversizedPixmapRule(qint64 pixelThreshold) : m_pixelThreshold(pixelThreshold) {}

    quint64 features() const override { return 1ULL << ProfilePixmapCache; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() != PixmapCacheEvent || type.detailType() != PixmapSizeKnown)
            return;

        const qint64 pixels = qint64(event.number<qint32>(0)) * qint64(event.number<qint32>(1));
        Data &data = m_data[event.typeIndex()];
        if (pixels > data.pixels) {
            data.pixels = pixels;
            data.width = event.number<qint32>(0);
            data.height = event.number<qint32>(1);
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            if (it->pixels < m_pixelThreshold)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "oversized-pixmap";
            finding.severity = Finding::Info;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.occurrences = 1;
            finding.what = Tr::tr("Image is decoded at %1x%2.").arg(it->width).arg(it->height);
            finding.why = Tr::tr("The cache keeps the image at the size it was decoded, no matter "
                                 "how small it is drawn, and decoding itself costs more the "
                                 "larger the image is.");
            finding.suggestion = Tr::tr("Set sourceSize on the Image to the size it is actually "
                                        "drawn at, so the image is decoded at that size.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override { m_data.clear(); }

private:
    struct Data {
        qint64 pixels = 0;
        int width = 0;
        int height = 0;
    };

    QHash<int, Data> m_data;
    const qint64 m_pixelThreshold;
};

// Reports bindings and functions whose cost, spread over the frames that were rendered,
// eats a noticeable part of a frame.
class PerFrameCostRule final : public FindingRule
{
public:
    explicit PerFrameCostRule(qint64 budgetNs) : m_budgetNs(budgetNs) {}

    quint64 features() const override
    {
        return (1ULL << ProfileBinding) | (1ULL << ProfileJavaScript)
               | (1ULL << ProfileAnimations);
    }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() == Event && type.detailType() == AnimationFrame) {
            if (m_frameThread.accepts(event))
                ++m_frames;
            return;
        }

        const RangeType rangeType = type.rangeType();
        if (rangeType != Binding && rangeType != Javascript)
            return;

        const int typeIndex = event.typeIndex();
        switch (event.rangeStage()) {
        case RangeStart:
            if (typeIndex < 0)
                break;
            if (typeIndex >= m_data.size())
                m_data.resize(typeIndex + 1);
            m_data[typeIndex].starts.push(event.timestamp());
            break;
        case RangeEnd: {
            if (typeIndex < 0 || typeIndex >= m_data.size())
                break;
            Data &data = m_data[typeIndex];
            if (data.starts.isEmpty())
                break;
            data.totalNs += event.timestamp() - data.starts.pop();
            data.ended = true;
            break;
        }
        default:
            break;
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        if (m_frames == 0)
            return findings;

        for (int typeIndex = 0; typeIndex < m_data.size(); ++typeIndex) {
            const Data &data = m_data.at(typeIndex);
            if (!data.ended)
                continue;
            const qint64 perFrame = data.totalNs / m_frames;
            if (perFrame < m_budgetNs)
                continue;

            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "per-frame-cost";
            finding.severity = Finding::Warning;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.costNs = data.totalNs;
            finding.occurrences = m_frames;
            finding.what = Tr::tr("Costs %1 ms per rendered frame (%2 ms over %n frame(s)).",
                                  nullptr, m_frames)
                               .arg(perFrame / 1000000.0, 0, 'f', 2)
                               .arg(data.totalNs / 1000000);
            finding.why = Tr::tr("Work of this size, repeated for every frame, leaves less of the "
                                 "frame for everything else that has to happen in it.");
            finding.suggestion = Tr::tr("Look for a value that is recomputed although it did not "
                                        "change, and keep the result instead of deriving it again "
                                        "each frame.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override
    {
        m_data.clear();
        m_frameThread.clear();
        m_frames = 0;
    }

private:
    // By type index: this rule sees every binding and function call.
    struct Data
    {
        QStack<qint64> starts;
        qint64 totalNs = 0;
        bool ended = false;
    };
    QList<Data> m_data;
    FrameThread m_frameThread;
    int m_frames = 0;
    const qint64 m_budgetNs;
};

// Reports a single call that ran long enough to be noticed on its own. The per-frame rule
// needs rendered frames to divide by and says nothing about a trace without them; a call
// that holds the thread for a quarter of a second is a stall either way.
class BlockingCallRule final : public FindingRule
{
public:
    explicit BlockingCallRule(qint64 thresholdNs) : m_thresholdNs(thresholdNs) {}

    quint64 features() const override
    {
        return (1ULL << ProfileBinding) | (1ULL << ProfileJavaScript)
               | (1ULL << ProfileHandlingSignal);
    }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        const RangeType rangeType = type.rangeType();
        if (rangeType != Binding && rangeType != Javascript && rangeType != HandlingSignal)
            return;

        const int typeIndex = event.typeIndex();
        switch (event.rangeStage()) {
        case RangeStart:
            m_stack.push(typeIndex, rangeType, event.timestamp());
            break;
        case RangeEnd: {
            const std::optional<RangeStack::Frame> frame = m_stack.pop(typeIndex);
            // Only the outermost call is reported. What nests inside it is part of the
            // same block, and naming every level of it states the same stall repeatedly.
            if (!frame || !m_stack.isEmpty())
                break;

            const qint64 durationNs = event.timestamp() - frame->startTime;
            if (durationNs < m_thresholdNs)
                break;

            Data &data = m_data[typeIndex];
            data.longestNs = qMax(data.longestNs, durationNs);
            ++data.count;
            break;
        }
        default:
            break;
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "blocking-call";
            finding.severity = Finding::Warning;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.costNs = it->longestNs;
            finding.occurrences = it->count;
            finding.what = it->count == 1
                               ? Tr::tr("Blocks for %1 ms in one call.")
                                     .arg(it->longestNs / 1000000)
                               : Tr::tr("Blocks for up to %1 ms in %n call(s).", nullptr, it->count)
                                     .arg(it->longestNs / 1000000);
            finding.why = Tr::tr("Nothing else runs on this thread while the call does, so no "
                                 "input is answered and no frame is drawn until it returns.");
            finding.suggestion = Tr::tr("Split the work so that each part fits in a frame, move "
                                        "it to C++ where it can run on another thread, or hand "
                                        "it to a WorkerScript.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override
    {
        m_data.clear();
        m_stack.clear();
    }

private:
    struct Data {
        qint64 longestNs = 0;
        int count = 0;
    };

    QHash<int, Data> m_data;
    RangeStack m_stack;
    const qint64 m_thresholdNs;
};

// Reports what ran during the frames that took too long. An average spread over every
// frame hides work that all lands in a few of them, and those few are the ones the user
// sees as a stutter.
class FrameJankRule final : public FindingRule
{
public:
    explicit FrameJankRule(qint64 budgetNs) : m_budgetNs(budgetNs) {}

    quint64 features() const override
    {
        return (1ULL << ProfileAnimations) | (1ULL << ProfileBinding)
               | (1ULL << ProfileJavaScript) | (1ULL << ProfileCreating)
               | (1ULL << ProfileHandlingSignal);
    }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() == Event && type.detailType() == AnimationFrame) {
            loadFrame(event);
            return;
        }

        const RangeType rangeType = type.rangeType();
        if (rangeType != Binding && rangeType != Javascript && rangeType != Creating
            && rangeType != HandlingSignal) {
            return;
        }

        const int typeIndex = event.typeIndex();
        switch (event.rangeStage()) {
        case RangeStart:
            m_stack.push(typeIndex, rangeType, event.timestamp());
            break;
        case RangeEnd: {
            const std::optional<RangeStack::Frame> frame = m_stack.pop(typeIndex);
            // Only the outermost range counts: the frame waited for as long as it ran, and
            // adding what nests inside it would charge the same time twice.
            if (frame && m_stack.isEmpty())
                m_pending[typeIndex] += event.timestamp() - frame->startTime;
            break;
        }
        default:
            break;
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            // A location has to account for a frame's worth of work before it is worth
            // naming: in a long trace almost everything appears in some slow frame.
            if (it->costNs < m_budgetNs)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "frame-jank";
            finding.severity = Finding::Warning;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.costNs = it->costNs;
            finding.occurrences = it->frames;
            finding.what = Tr::tr("Runs for %1 ms inside %2 of the %n frame(s) that took longer "
                                  "than %3 ms.", nullptr, m_jankyFrames)
                               .arg(it->costNs / 1000000).arg(it->frames)
                               .arg(m_budgetNs / 1000000);
            finding.why = Tr::tr("The frame could not be finished while this was running, so it "
                                 "stayed on screen for as long as the work took. Work that lands "
                                 "in a few frames is seen as a jump, not as a slowdown.");
            finding.suggestion = Tr::tr("Move the work out of the frame it happens in: do it "
                                        "before it is needed, spread it over several frames, or "
                                        "let it run asynchronously and show the result when it "
                                        "arrives.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override
    {
        m_data.clear();
        m_pending.clear();
        m_stack.clear();
        m_frameThread.clear();
        m_lastFrameEnd = -1;
        m_jankyFrames = 0;
    }

private:
    void loadFrame(const QmlEvent &event)
    {
        if (!m_frameThread.accepts(event))
            return;

        // The profiler timestamps a frame when it ends, so what was collected since the
        // previous frame is what this one waited for. A range that started even earlier is
        // charged to the frame it ended in, which is the frame it finally released.
        if (m_lastFrameEnd >= 0 && event.timestamp() - m_lastFrameEnd > m_budgetNs) {
            ++m_jankyFrames;
            for (auto it = m_pending.constBegin(), end = m_pending.constEnd(); it != end; ++it) {
                Data &data = m_data[it.key()];
                data.costNs += it.value();
                ++data.frames;
            }
        }
        m_lastFrameEnd = event.timestamp();
        m_pending.clear();
    }

    struct Data {
        qint64 costNs = 0;
        int frames = 0;
    };

    QHash<int, Data> m_data;
    QHash<int, qint64> m_pending;
    RangeStack m_stack;
    FrameThread m_frameThread;
    qint64 m_lastFrameEnd = -1;
    int m_jankyFrames = 0;
    const qint64 m_budgetNs;
};

// Reports the code that allocates most on the JavaScript heap. Allocating is cheap; the
// collection it forces is not, and that runs at a moment the allocating code does not pick.
class MemoryChurnRule final : public FindingRule
{
public:
    explicit MemoryChurnRule(qint64 thresholdBytes) : m_thresholdBytes(thresholdBytes) {}

    quint64 features() const override
    {
        return (1ULL << ProfileMemory) | (1ULL << ProfileBinding) | (1ULL << ProfileJavaScript)
               | (1ULL << ProfileCreating) | (1ULL << ProfileHandlingSignal);
    }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() == MemoryAllocation) {
            // Heap pages are the allocator growing itself, not what a piece of code asked
            // for. The items are.
            if (type.detailType() != SmallItem && type.detailType() != LargeItem)
                return;

            const qint64 amount = event.number<qint64>(0);
            // A negative amount is a release, and it is reported wherever the collector
            // happened to run, not where the memory was taken.
            if (amount <= 0 || m_stack.isEmpty())
                return;

            Data &data = m_data[m_stack.top().typeIndex];
            data.bytes += amount;
            ++data.count;
            return;
        }

        const RangeType rangeType = type.rangeType();
        if (rangeType != Binding && rangeType != Javascript && rangeType != Creating
            && rangeType != HandlingSignal) {
            return;
        }

        switch (event.rangeStage()) {
        case RangeStart:
            m_stack.push(event.typeIndex(), rangeType, event.timestamp());
            break;
        case RangeEnd:
            m_stack.pop(event.typeIndex());
            break;
        default:
            break;
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            if (it->bytes < m_thresholdBytes)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "memory-churn";
            finding.severity = Finding::Info;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.occurrences = it->count;
            finding.what = Tr::tr("Takes %1 from the JavaScript heap in %n allocation(s).", nullptr,
                                  it->count)
                               .arg(Timeline::formatDataSize(it->bytes));
            finding.why = Tr::tr("Every object taken from the heap has to be collected again. "
                                 "The collection does not happen here, but in whichever frame the "
                                 "heap happens to fill up, which is why it is hard to place.");
            finding.suggestion = Tr::tr("Look for objects, arrays or strings built anew on each "
                                        "call and keep them instead, and move bulk data handling "
                                        "to C++ where it does not touch this heap.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override
    {
        m_data.clear();
        m_stack.clear();
    }

private:
    struct Data {
        qint64 bytes = 0;
        int count = 0;
    };

    QHash<int, Data> m_data;
    RangeStack m_stack;
    const qint64 m_thresholdBytes;
};

// Reports a component built over and over. What one instance costs to build is paid again
// for every instance, so the finding names what builds them, which decides how to build
// fewer, and what in an instance costs most, which decides how to make each one cheaper.
class RepeatedCreationRule final : public FindingRule
{
public:
    explicit RepeatedCreationRule(int minCount,
                                  const std::shared_ptr<SharedInstanceTracker> &tracker
                                  = std::make_shared<SharedInstanceTracker>())
        : m_tracker(tracker)
        , m_minCount(minCount)
    {
        m_tracker->subscribe(this, [this](const InstanceTracker::Instance &instance) {
            add(instance);
        });
    }

    quint64 features() const override { return InstanceTracker::features(); }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        m_tracker->loadEvent(this, event, type);
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        m_tracker->finish();

        QList<Finding> findings;
        for (auto it = m_components.constBegin(), end = m_components.constEnd(); it != end;
             ++it) {
            if (it->instances < m_minCount || it->instances <= 0)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);
            findings.append(finding(manager, typeIndex, *it));
        }
        return findings;
    }

    void clear() override
    {
        m_tracker->clear(this);
        m_components.clear();
    }

private:
    struct Component
    {
        int instances = 0;
        qint64 costNs = 0;
        QHash<InstanceTracker::Context, int> contexts;
        QHash<int, InstanceTracker::Part> parts;
    };

    void add(const InstanceTracker::Instance &instance)
    {
        if (instance.partOf >= 0) {
            // Built for an instance of another component, so it is a part of that one.
            Component &component = m_components[instance.partOf];
            component.costNs += instance.costNs;
            InstanceTracker::Part &part = component.parts[instance.rootType];
            part.costNs += instance.costNs;
            ++part.count;
            part.createdLater = true;
            return;
        }

        Component &component = m_components[instance.rootType];
        ++component.instances;
        component.costNs += instance.costNs;
        ++component.contexts[instance.context];
        for (auto it = instance.parts.constBegin(), end = instance.parts.constEnd(); it != end;
             ++it) {
            InstanceTracker::Part &part = component.parts[it.key()];
            part.costNs += it->costNs;
            part.count += it->count;
            part.nestedInstance = part.nestedInstance || it->nestedInstance;
            part.createdLater = part.createdLater || it->createdLater;
        }
    }

    QString partName(const QmlProfilerModelManager *manager, int typeIndex,
                     const InstanceTracker::Part &part) const
    {
        const QmlEventType &type = manager->eventType(typeIndex);
        if (part.nestedInstance)
            return Tr::tr("instances of %1 built inside it").arg(objectTypeName(type));
        if (part.createdLater)
            return Tr::tr("%1, created when first needed").arg(objectTypeName(type));
        return scriptName(type, m_tracker->name(typeIndex));
    }

    Finding finding(const QmlProfilerModelManager *manager, int typeIndex,
                    const Component &component) const
    {
        const QmlEventType &type = manager->eventType(typeIndex);
        const QString total = Timeline::formatTime(component.costNs);
        const QString each = Timeline::formatTime(component.costNs / component.instances);

        Finding finding;
        finding.ruleId = "repeated-creation";
        finding.severity = Finding::Info;
        finding.location = type.location();
        finding.typeIndex = typeIndex;
        finding.costNs = component.costNs;
        finding.occurrences = component.instances;

        QStringList suggestion;
        const InstanceTracker::Context context = dominantContext(component.contexts);
        if (context.kind != InstanceTracker::Context::Unknown) {
            QTC_ASSERT(context.typeIndex >= 0 && context.typeIndex < manager->numEventTypes(),
                       return finding);
        }
        switch (context.kind) {
        case InstanceTracker::Context::Owner: {
            const QmlEventType &owner = manager->eventType(context.typeIndex);
            const QString ownerName = objectTypeName(owner);
            const QString ownerLocation = owner.displayName();
            finding.what = Tr::tr("Built %n time(s) by the %1 at %2, %3 in total, %4 each.",
                                  nullptr, component.instances)
                               .arg(ownerName, ownerLocation, total, each);
            if (isReusingView(ownerName)) {
                finding.why = Tr::tr("The %1 at %2 builds an instance for each item that comes "
                                     "into view. Unless it reuses them, it builds a new one for "
                                     "every item scrolled to, and each costs %3 to build.")
                                  .arg(ownerName, ownerLocation, each);
                suggestion << Tr::tr("Unless it does already, set reuseItems: true on the %1, so "
                                     "that instances scrolled out of view are kept in a pool and "
                                     "handed out again.").arg(ownerName);
            } else if (reusesByDefault(ownerName)) {
                finding.why = Tr::tr("The %1 at %2 builds an instance for each cell that comes "
                                     "into view, and each costs %3 to build. It reuses them by "
                                     "default, so building this many usually means that "
                                     "reuseItems is turned off, or that the model is reset, "
                                     "which rebuilds them all.")
                                  .arg(ownerName, ownerLocation, each);
                suggestion << Tr::tr("Check that reuseItems is not turned off on the %1, and "
                                     "change its model in place rather than resetting it.")
                                  .arg(ownerName);
            } else if (buildsEveryEntry(ownerName)) {
                finding.why = Tr::tr("The %1 at %2 builds an instance for every entry of its "
                                     "model, all at once, and builds all of them again whenever "
                                     "the model is replaced. Each costs %3 to build.")
                                  .arg(ownerName, ownerLocation, each);
                suggestion << Tr::tr("Change the model in place rather than replacing it, for "
                                     "example with a ListModel instead of a JavaScript array or "
                                     "a number, or show the items in a ListView with reuseItems "
                                     "if only some of them are visible at a time.");
            } else if (ownerName == QLatin1String("Loader")) {
                finding.why = Tr::tr("The Loader at %1 builds its item anew whenever it becomes "
                                     "active or its source changes, and each build costs %2.")
                                  .arg(ownerLocation, each);
                suggestion << Tr::tr("If the item is needed again and again, keep it loaded and "
                                     "hide it, instead of deactivating the Loader or changing its "
                                     "source.");
            } else {
                finding.why = Tr::tr("The %1 at %2 builds these instances, and each costs %3 to "
                                     "build.").arg(ownerName, ownerLocation, each);
                suggestion << Tr::tr("Check whether the %1 can keep instances instead of building "
                                     "new ones.").arg(ownerName);
            }

            FindingDetail detail;
            detail.text = Tr::tr("Built by %1").arg(ownerName);
            detail.location = owner.location();
            detail.typeIndex = context.typeIndex;
            detail.occurrences = component.contexts.value(context);
            finding.details.append(detail);
            break;
        }
        case InstanceTracker::Context::Script: {
            const QmlEventType &script = manager->eventType(context.typeIndex);
            const QString name = scriptName(script, m_tracker->name(context.typeIndex));
            finding.what = Tr::tr("Built %n time(s) from JavaScript in %1 at %2, %3 in total, "
                                  "%4 each.", nullptr, component.instances)
                               .arg(name, script.displayName(), total, each);
            finding.why = Tr::tr("These instances are created from JavaScript, in %1 at %2. "
                                 "Building one costs %3: creating its objects, evaluating their "
                                 "bindings and completing them. No view is involved, so nothing "
                                 "reuses instances that are no longer needed.")
                              .arg(name, script.displayName(), each);
            suggestion << Tr::tr("Keep instances that are no longer needed and reuse them instead "
                                 "of destroying them and creating new ones, create them ahead of "
                                 "time or with incubateObject() so that the work is spread over "
                                 "several frames, or create fewer of them.");

            FindingDetail detail;
            detail.text = Tr::tr("Created in %1").arg(name);
            detail.location = script.location();
            detail.typeIndex = context.typeIndex;
            detail.occurrences = component.contexts.value(context);
            finding.details.append(detail);
            break;
        }
        case InstanceTracker::Context::Unknown:
            finding.what = Tr::tr("Built %n time(s), %1 in total, %2 each.", nullptr,
                                  component.instances)
                               .arg(total, each);
            finding.why = Tr::tr("Each instance costs %1 to build. The trace does not show what "
                                 "built them: no QML or JavaScript was running at the time, so "
                                 "it was C++ code.").arg(each);
            break;
        }

        QList<std::pair<int, InstanceTracker::Part>> parts;
        for (auto it = component.parts.constBegin(), end = component.parts.constEnd(); it != end;
             ++it) {
            if (it->costNs > 0)
                parts.append({it.key(), it.value()});
        }
        std::sort(parts.begin(), parts.end(), [](const auto &a, const auto &b) {
            return a.second.costNs > b.second.costNs;
        });

        QStringList costliest;
        for (const auto &[partType, part] : parts.mid(0, maxDetails)) {
            const QString name = partName(manager, partType, part);
            costliest << Tr::tr("%1 at %2 (%3 each)")
                             .arg(name, manager->eventType(partType).displayName(),
                                  Timeline::formatTime(part.costNs / component.instances));

            FindingDetail detail;
            detail.text = name;
            detail.location = locationOf(manager, partType);
            detail.typeIndex = partType;
            detail.costNs = part.costNs;
            detail.occurrences = part.count;
            finding.details.append(detail);
        }
        if (costliest.isEmpty()) {
            suggestion << Tr::tr("To make each instance cheaper, give it fewer objects and "
                                 "bindings.");
        } else {
            suggestion << Tr::tr("To make each instance cheaper, start with what takes longest "
                                 "to build in it: %1.").arg(costliest.join(QLatin1String(", ")));
        }

        // Which parts are actually shown is not in the trace: the profiler records that a
        // binding ran, not what it evaluated to. Where visibility is a binding, though, the
        // part is shown only under some condition, and that is worth checking.
        QList<int> conditionTypes;
        for (auto it = component.parts.constBegin(), end = component.parts.constEnd(); it != end;
             ++it) {
            const QString name = m_tracker->name(it.key());
            if (name == QLatin1String("visible") || name == QLatin1String("opacity"))
                conditionTypes.append(it.key());
        }
        std::sort(conditionTypes.begin(), conditionTypes.end(), [manager](int a, int b) {
            const QmlEventLocation first = manager->eventType(a).location();
            const QmlEventLocation second = manager->eventType(b).location();
            if (first.filename() != second.filename())
                return first.filename() < second.filename();
            return first.line() < second.line();
        });
        QStringList conditions;
        for (int conditionType : conditionTypes.mid(0, maxDetails)) {
            conditions << Tr::tr("%1 at %2").arg(m_tracker->name(conditionType),
                                                 manager->eventType(conditionType).displayName());
        }
        if (!conditions.isEmpty()) {
            suggestion << Tr::tr("Some of its parts are shown only under a condition (%1). The "
                                 "trace does not record what such a binding evaluates to; if a "
                                 "part is rarely shown, build it with a Loader only when it is "
                                 "needed.").arg(conditions.join(QLatin1String(", ")));
        }

        finding.suggestion = suggestion.join(QLatin1Char(' '));
        return finding;
    }

    static constexpr int maxDetails = 3;

    QHash<int, Component> m_components;
    const std::shared_ptr<SharedInstanceTracker> m_tracker;
    const int m_minCount;
};

// Reports a binding evaluated far more often than anything can be looked at. One
// evaluation may be too cheap to show up in any cost; that it happens that often is the
// finding, because something is changing what the binding reads at that rate.
class BindingChurnRule final : public FindingRule
{
public:
    explicit BindingChurnRule(int minCount) : m_minCount(minCount) {}

    quint64 features() const override { return 1ULL << ProfileBinding; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.rangeType() == Binding)
            m_totals.loadEvent(event, Binding);
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_totals.begin(), end = m_totals.end(); it != end; ++it) {
            if (it->count < m_minCount)
                continue;

            const int typeIndex = it.key();
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "binding-churn";
            finding.severity = Finding::Info;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.costNs = it->totalNs;
            finding.occurrences = it->count;
            finding.what = Tr::tr("Re-evaluated %n time(s), %1 ms in total.", nullptr, it->count)
                               .arg(it->totalNs / 1000000);
            finding.why = Tr::tr("A binding runs again whenever anything it reads changes. At "
                                 "this rate it is reading something that changes continuously, "
                                 "and each run can invalidate the bindings that read its result.");
            finding.suggestion = Tr::tr("Find the value that changes that often and stop the "
                                        "binding from reading it directly: derive it once into a "
                                        "property the binding reads, or express the change as an "
                                        "animation or a Behavior instead.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override { m_totals.clear(); }

private:
    RangeTotals m_totals;
    const int m_minCount;
};

// Reports an image loaded more than once. A decoded image stays in the cache only while
// something holds it, so a second load means it was dropped in between and decoded again.
class PixmapReloadRule final : public FindingRule
{
public:
    explicit PixmapReloadRule(int minCount) : m_minCount(minCount) {}

    quint64 features() const override { return 1ULL << ProfilePixmapCache; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() != PixmapCacheEvent)
            return;

        // The stages of one load are separate event types, so they are collected by the
        // source they refer to rather than by type index.
        const QString source = type.location().filename();
        switch (type.detailType()) {
        case PixmapLoadingStarted: {
            Data &data = m_data[source];
            data.typeIndex = event.typeIndex();
            data.lastStart = event.timestamp();
            ++data.count;
            break;
        }
        case PixmapLoadingFinished: {
            auto it = m_data.find(source);
            if (it == m_data.end() || it->lastStart < 0)
                break;
            it->totalNs += event.timestamp() - it->lastStart;
            it->lastStart = -1;
            break;
        }
        case PixmapLoadingError:
            // A source that fails is reported as an error, and each retry of it would
            // otherwise be counted here as a reload as well.
            m_failed.insert(source);
            break;
        default:
            break;
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_data.constBegin(), end = m_data.constEnd(); it != end; ++it) {
            if (it->count < m_minCount || m_failed.contains(it.key()))
                continue;

            const int typeIndex = it->typeIndex;
            QTC_ASSERT(typeIndex >= 0 && typeIndex < manager->numEventTypes(), continue);

            Finding finding;
            finding.ruleId = "pixmap-reload";
            finding.severity = Finding::Info;
            finding.location = manager->eventType(typeIndex).location();
            finding.typeIndex = typeIndex;
            finding.costNs = it->totalNs;
            finding.occurrences = it->count;
            finding.what = Tr::tr("Image is loaded %n time(s).", nullptr, it->count);
            finding.why = Tr::tr("The decoded image is kept only while something refers to it. "
                                 "Loading the same source again means it was dropped in between, "
                                 "so reading and decoding it is paid for once more.");
            finding.suggestion = Tr::tr("Keep the item that uses the image alive instead of "
                                        "destroying and recreating it, for example by hiding it "
                                        "rather than unloading its Loader, or raise the image "
                                        "cache so the entry survives until it is needed again.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override
    {
        m_data.clear();
        m_failed.clear();
    }

private:
    struct Data {
        qint64 totalNs = 0;
        qint64 lastStart = -1;
        int typeIndex = -1;
        int count = 0;
    };

    QHash<QString, Data> m_data;
    QSet<QString> m_failed;
    const int m_minCount;
};

FindingRules defaultFindingRules()
{
    // Thresholds are user settings: what counts as too slow depends on the target the
    // application is meant to run on, not on the trace.
    const QmlProfilerSettings &settings = globalSettings();

    FindingRules rules;
    rules.push_back(std::make_unique<FirstUseCompileRule>(
        settings.findingsCompileThresholdMs() * 1000000ll));
    rules.push_back(std::make_unique<PixmapLoadErrorRule>());
    const auto instanceTracker = std::make_shared<SharedInstanceTracker>();
    rules.push_back(std::make_unique<SyncViewLoadRule>(
        settings.findingsSyncLoadThresholdMs() * 1000000ll, instanceTracker));
    rules.push_back(std::make_unique<PeriodicHandlerRule>(
        settings.findingsPeriodicMinCount(),
        settings.findingsPeriodicDeviationPercent() / 100.0));
    rules.push_back(std::make_unique<OversizedPixmapRule>(
        qint64(settings.findingsPixmapMegapixels() * 1000000.0)));
    rules.push_back(std::make_unique<PerFrameCostRule>(
        settings.findingsPerFrameBudgetUs() * 1000ll));
    rules.push_back(std::make_unique<BlockingCallRule>(
        settings.findingsBlockingCallThresholdMs() * 1000000ll));
    rules.push_back(std::make_unique<FrameJankRule>(
        settings.findingsJankFrameThresholdMs() * 1000000ll));
    rules.push_back(std::make_unique<MemoryChurnRule>(
        settings.findingsMemoryChurnKb() * 1024ll));
    rules.push_back(std::make_unique<RepeatedCreationRule>(settings.findingsCreationMinCount(),
                                                           instanceTracker));
    rules.push_back(std::make_unique<BindingChurnRule>(settings.findingsBindingMinCount()));
    rules.push_back(std::make_unique<PixmapReloadRule>(settings.findingsPixmapReloadMinCount()));
    return rules;
}

QmlProfilerFindingsModel::QmlProfilerFindingsModel(QmlProfilerModelManager *modelManager,
                                                   FindingRules rules)
    : m_modelManager(modelManager)
    , m_rules(std::move(rules))
{
    quint64 features = 0;
    for (const auto &rule : m_rules) {
        features |= rule->features();
        for (int feature = 0; feature < MaximumProfileFeature; ++feature) {
            if (rule->features() & (1ULL << feature))
                m_rulesByFeature[feature].append(rule.get());
        }
    }

    modelManager->qmlLoaders.append({features,
                                     std::bind(&QmlProfilerFindingsModel::loadEvent, this,
                                               std::placeholders::_1, std::placeholders::_2)});
    modelManager->registerFeatures(features,
                                   std::bind(&QmlProfilerFindingsModel::beginResetModel, this),
                                   std::bind(&QmlProfilerFindingsModel::finalize, this),
                                   std::bind(&QmlProfilerFindingsModel::clear, this));
}

QmlProfilerFindingsModel::~QmlProfilerFindingsModel() = default;

void QmlProfilerFindingsModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    // Every event goes only to the rules that asked for its feature. They pick the events of
    // it they care about themselves.
    const quint8 feature = type.feature();
    if (feature >= MaximumProfileFeature)
        return;
    for (FindingRule *rule : std::as_const(m_rulesByFeature[feature]))
        rule->loadEvent(event, type);
}

void QmlProfilerFindingsModel::finalize()
{
    m_findings.clear();
    for (const auto &rule : m_rules)
        m_findings.append(rule->finalize(m_modelManager));

    std::stable_sort(m_findings.begin(), m_findings.end(),
                     [](const Finding &a, const Finding &b) {
        if (a.severity != b.severity)
            return a.severity > b.severity;
        return a.costNs > b.costNs;
    });

    endResetModel();
}

void QmlProfilerFindingsModel::clear()
{
    beginResetModel();
    for (const auto &rule : m_rules)
        rule->clear();
    m_findings.clear();
    endResetModel();
}

// Findings are the top-level rows, and the details of a finding are its children. A child
// carries the row of its finding, plus one, as internal id; a finding carries 0.
QModelIndex QmlProfilerFindingsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return {};
    return createIndex(row, column, parent.isValid() ? quintptr(parent.row() + 1) : quintptr(0));
}

QModelIndex QmlProfilerFindingsModel::parent(const QModelIndex &child) const
{
    if (!child.isValid() || child.internalId() == 0)
        return {};
    return createIndex(int(child.internalId() - 1), 0, quintptr(0));
}

int QmlProfilerFindingsModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_findings.count();
    if (parent.internalId() != 0 || parent.column() != 0 || parent.row() >= m_findings.count())
        return 0;
    return m_findings.at(parent.row()).details.count();
}

int QmlProfilerFindingsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return MaxColumn;
}

static QString severityLabel(Finding::Severity severity)
{
    switch (severity) {
    case Finding::Critical:
        return Tr::tr("Critical");
    case Finding::Warning:
        return Tr::tr("Warning");
    case Finding::Info:
        break;
    }
    return Tr::tr("Info");
}

static QString locationText(const QmlEventLocation &location)
{
    // Compiling ranges and pixmap events carry no line: the trace reports 0 for them, and
    // QML lines are 1-based, so anything below 1 is "no line".
    return location.line() > 0
               ? QString::fromLatin1("%1:%2").arg(location.filename()).arg(location.line())
               : location.filename();
}

static QVariant detailData(const Finding &finding, const FindingDetail &detail, int column,
                           int role)
{
    switch (role) {
    case Qt::DisplayRole:
        switch (column) {
        case QmlProfilerFindingsModel::ColumnFinding:
            return detail.text;
        case QmlProfilerFindingsModel::ColumnLocation:
            return locationText(detail.location);
        case QmlProfilerFindingsModel::ColumnCost:
            return detail.costNs > 0 ? Timeline::formatTime(detail.costNs) : QString();
        case QmlProfilerFindingsModel::ColumnOccurrences:
            return detail.occurrences > 0 ? QVariant(detail.occurrences) : QVariant();
        default:
            return {};
        }
    case QmlProfilerFindingsModel::SortRole:
        switch (column) {
        case QmlProfilerFindingsModel::ColumnCost:
            return detail.costNs;
        case QmlProfilerFindingsModel::ColumnOccurrences:
            return detail.occurrences;
        default:
            return detailData(finding, detail, column, Qt::DisplayRole);
        }
    case QmlProfilerFindingsModel::TypeIdRole:
        return detail.typeIndex;
    case QmlProfilerFindingsModel::FilenameRole:
        return detail.location.filename();
    case QmlProfilerFindingsModel::LineRole:
        return detail.location.line();
    case QmlProfilerFindingsModel::ColumnRole:
        return detail.location.column();
    case QmlProfilerFindingsModel::RuleIdRole:
        return finding.ruleId;
    default:
        return {};
    }
}

QVariant QmlProfilerFindingsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (index.internalId() != 0) {
        const int findingRow = int(index.internalId() - 1);
        if (findingRow >= m_findings.count())
            return {};
        const Finding &finding = m_findings.at(findingRow);
        if (index.row() >= finding.details.count())
            return {};
        return detailData(finding, finding.details.at(index.row()), index.column(), role);
    }

    if (index.row() >= m_findings.count())
        return {};

    const Finding &finding = m_findings.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ColumnSeverity:
            return severityLabel(finding.severity);
        case ColumnFinding:
            return finding.what;
        case ColumnLocation:
            return locationText(finding.location);
        case ColumnCost:
            return finding.costNs > 0 ? QString::fromLatin1("%1 ms").arg(finding.costNs / 1000000)
                                      : QString();
        case ColumnOccurrences:
            return finding.occurrences;
        default:
            return {};
        }
    case Qt::ToolTipRole:
        return QString(finding.why + QLatin1String("\n\n") + finding.suggestion);
    case SortRole:
        switch (index.column()) {
        case ColumnSeverity:
            return finding.severity;
        case ColumnCost:
            return finding.costNs;
        case ColumnOccurrences:
            return finding.occurrences;
        default:
            return data(index, Qt::DisplayRole);
        }
    case TypeIdRole:
        return finding.typeIndex;
    case FilenameRole:
        return finding.location.filename();
    case LineRole:
        return finding.location.line();
    case ColumnRole:
        return finding.location.column();
    case WhyRole:
        return finding.why;
    case SuggestionRole:
        return finding.suggestion;
    case RuleIdRole:
        return finding.ruleId;
    default:
        return {};
    }
}

QVariant QmlProfilerFindingsModel::headerData(int section, Qt::Orientation orientation,
                                              int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case ColumnSeverity:
        return Tr::tr("Severity");
    case ColumnFinding:
        return Tr::tr("Finding");
    case ColumnLocation:
        return Tr::tr("Location");
    case ColumnCost:
        return Tr::tr("Cost");
    case ColumnOccurrences:
        return Tr::tr("Count");
    default:
        return {};
    }
}

bool findingIsInSource(const QModelIndex &index)
{
    return index.data(QmlProfilerFindingsModel::LineRole).toInt() > 0
           && !index.data(QmlProfilerFindingsModel::FilenameRole).toString().isEmpty();
}

} // namespace Profiler::Internal
