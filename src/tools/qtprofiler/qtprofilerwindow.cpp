// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtprofilerwindow.h"

#include <profiler/callstacksampler.h>
#include "mainsidebar.h"
#include "qtprofilerrpc.h"
#include "qtprofilersettings.h"
#include "recordingpage.h"
#include <profiler/qmlprofilersampler.h>
#include <profiler/sampler.h>
#include <profiler/samplerviewmanager.h>
#include "welcomepage.h"

#include <utils/aspects.h>
#include <utils/layoutbuilder.h>

#include <profiler/ctfplainviewmanager.h>
#include <profiler/profilertr.h>
#include <profiler/sampletrace.h>
#include <profiler/qmlprofilerconstants.h>
#include <profiler/qmlprofilerplainviewmanager.h>

#include <coreplugin/minisplitter.h>

#include <utils/commandline.h>
#include <utils/fancymainwindow.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/progressindicator.h>
#include <utils/qtcprocess.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QtTaskTree/QBarrier>
#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QEventLoop>
#include <QMessageBox>
#include <QPointer>
#include <QSplitter>
#include <QStackedWidget>
#include <QTime>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>

#include <atomic>
#include <iostream>
#include <optional>
#include <memory>

using namespace Profiler;
using namespace QmlProfiler::Internal;
using namespace Utils;
using namespace Utils::StyleHelper;

using namespace Qt::StringLiterals;

using namespace std::chrono;

namespace QtProfiler {

// A trace file format determines which manager and view set are used. Chrome
// Trace Format and Common Trace Format both render through the CTF views.
enum class Format { Qml, Ctf, Sampler };

// One set of dock widgets, plus its (untabbed) range-details dock. The views
// are tabbed onto the first by default; when stackVertically is set they are
// split below it instead (the sampler stacks Call Stacks below CPU Usage).
struct ViewGroup
{
    QList<QDockWidget *> docks;         // List with original dock widgets order
    QDockWidget *detailsDock = nullptr; // Range details, docked to the right (not tabbed)
    bool stackVertically = false;
};

class WindowPrivate : public QObject
{
public:
    WindowPrivate(Window *window = nullptr);

    void showOpenFileDialog();
    void showOpenCtfDirDialog();
    void selectBackend(int index);
    bool selectBackendByName(const QString &name);
    void startRecording();
    void startTimedRecording(milliseconds duration);
    void beginRecording(const QString &displayName);
    void finishRecording();
    void stopAndWaitForRecording();

    void onError(const QString &error);
    void onLoadFinished();
    void onGotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber,
                              const QString &module = {}, quint64 offset = 0);

    void addDocksForViews();
    void resetLayout(Format format);
    void resetActiveLayout() { resetLayout(activeFormat); }
    void setActiveFormat(Format format);
    void clearTrace();
    void doLoad(const Utils::FilePath &filePath);
    void setTraceDuration(milliseconds ms);
    milliseconds activeTraceDuration() const;

    static void openHelpInBrowser();

    Window *q = nullptr;
    QStackedWidget *rightPane = nullptr;  // Welcome / recording page or trace area.
    WelcomePage *welcomePage = nullptr;   // Shown while no trace is loaded.
    RecordingPage *recordingPage = nullptr; // Shown while a recording is running.
    FancyMainWindow *traceArea = nullptr; // Hosts the trace view docks (right pane).
    MainSidebar *sidebar = nullptr;       // List of opened traces (left pane).
    QmlProfilerPlainViewManager *qmlManager;
    CtfPlainViewManager *ctfManager;
    SamplerViewManager *samplerManager;
    Format activeFormat = Format::Qml;
    QString lastLoadError;
    QLabel *traceDurationLabel = nullptr;
    ProgressIndicator *progressIndicator;
    ViewGroup qmlGroup;
    ViewGroup ctfGroup;
    ViewGroup samplerGroup;
    bool recording = false;
    bool processingShown = false;                 // setProcessing() already called this session.
    bool closing = false;                         // Set while waiting for shutdown.
    std::vector<std::unique_ptr<Sampler>> backends; // Available profiling backends.
    Sampler *sampler = nullptr;                   // The selected backend (owned by `backends`).
    std::shared_ptr<RecordingSession> session;    // Non-null while recording.
    QtTaskTree::QSingleTaskTreeRunner recordingRunner;
    QTimer *processingPoll = nullptr;
    std::optional<milliseconds> recordDuration;   // Set by --record-for; auto-stop span.
    bool recordDurationArmed = false;             // Stop timer armed once capture started.
};

WindowPrivate::WindowPrivate(Window *window)
    : QObject(window)
    , q(window)
{
    traceArea = new FancyMainWindow(q);
    traceArea->setDockNestingEnabled(true);
    traceArea->setDocumentMode(true);

    sidebar = new MainSidebar(q);

    welcomePage = new WelcomePage(q);
    recordingPage = new RecordingPage(q);

    rightPane = new QStackedWidget(q);
    rightPane->addWidget(welcomePage);
    rightPane->addWidget(recordingPage);
    rightPane->addWidget(traceArea);
    rightPane->setCurrentWidget(welcomePage);

    connect(welcomePage, &WelcomePage::startRecordingRequested,
            this, &WindowPrivate::startRecording);
    connect(welcomePage, &WelcomePage::backendChanged, this, &WindowPrivate::selectBackend);

    // The native call-stack sampler first, then the QML-protocol profiler. Each
    // backend persists and renders its own settings, and decides how to start
    // (launch / attach / connect) from those settings in createSession().
    backends.push_back(std::make_unique<CallStackSampler>());
    backends.push_back(std::make_unique<QmlProfilerSampler>());

    QStringList backendNames;
    int defaultBackend = -1;
    for (int i = 0; i < int(backends.size()); ++i) {
        if (SamplerSettings *s = backends[i]->settings())
            s->readSettings();
        backendNames << backends[i]->displayName();
        if (defaultBackend < 0 && backends[i]->isAvailable())
            defaultBackend = i;
    }
    if (defaultBackend < 0)
        defaultBackend = 0;
    welcomePage->setBackends(backendNames, defaultBackend);
    selectBackend(defaultBackend);

    // A command line passed on the CLI (--launch) seeds the active backend's
    // launch settings so it can be started straight away.
    if (const FilePath exe = settings().recordExecutable(); !exe.isEmpty()) {
        if (SamplerSettings *s = sampler->settings()) {
            s->executable.setValue(exe);
            s->arguments.setValue(settings().recordArguments());
        }
    }

    processingPoll = new QTimer(this);
    processingPoll->setInterval(50);
    connect(processingPoll, &QTimer::timeout, this, [this] {
        if (!session)
            return;
        // The recipe (or the user) may set stop on its own (e.g. the target exited);
        // reflect the switch to post-processing exactly once.
        if (session->stop.load(std::memory_order_relaxed) && !processingShown) {
            processingShown = true;
            recordingPage->setProcessing();
        }
        // For --record-for: once capture is actually live, start the span clock
        // (exactly once) so launch/connect time isn't counted against it.
        if (recordDuration && !recordDurationArmed
                && session->started.load(std::memory_order_relaxed)) {
            recordDurationArmed = true;
            QTimer::singleShot(*recordDuration, this, [this] {
                if (session)
                    session->stop.store(true);
            });
        }
        recordingPage->setProgress(session->progress.load(std::memory_order_relaxed));
    });

    connect(recordingPage, &RecordingPage::stopRequested, this, [this] {
        if (session)
            session->stop.store(true);
    });

    qmlManager = new QmlProfilerPlainViewManager(traceArea);
    ctfManager = new CtfPlainViewManager(traceArea);
    samplerManager = new SamplerViewManager(traceArea);

    progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Large);
    progressIndicator->hide();

    connect(sidebar, &MainSidebar::traceActivated, this, &WindowPrivate::doLoad);

    connect(qmlManager, &QmlProfilerPlainViewManager::error, this, &WindowPrivate::onError);
    connect(qmlManager, &QmlProfilerPlainViewManager::loadFinished,
            this, &WindowPrivate::onLoadFinished);
    connect(qmlManager, &QmlProfilerPlainViewManager::gotoSourceLocation, this,
            [this](const QString &file, int line, int column) {
                onGotoSourceLocation(file, line, column);
            });

    connect(ctfManager, &CtfPlainViewManager::error, this, &WindowPrivate::onError);
    connect(ctfManager, &CtfPlainViewManager::loadFinished, this, &WindowPrivate::onLoadFinished);

    connect(samplerManager, &SamplerViewManager::error, this, &WindowPrivate::onError);
    connect(samplerManager, &SamplerViewManager::loadFinished,
            this, &WindowPrivate::onLoadFinished);
    connect(samplerManager, &SamplerViewManager::gotoSourceLocation,
            this, &WindowPrivate::onGotoSourceLocation);
}

void WindowPrivate::showOpenFileDialog()
{
    // "metadata" matches a Common Trace Format metadata file, which selects the
    // containing directory as the CTF2 trace (see loadTraceFile).
    const QString filter = Tr::tr("Traces (*.qtd *.qzip *.json metadata)") + ";;"_L1
                           + Tr::tr("QML Trace Files (*.qtd *.qzip)") + ";;"_L1
                           + Tr::tr("Chrome Trace Format Files (*.json)") + ";;"_L1
                           + Tr::tr("Common Trace Format Metadata Files (metadata)") + ";;"_L1
                           + Tr::tr("All Files (*)");
    const FilePath filePath
        = FileUtils::getOpenFilePath(Tr::tr("Load Trace"), settings().lastTraceFile(), filter);

    if (!filePath.isEmpty())
        q->loadTraceFile(filePath);
}

void WindowPrivate::showOpenCtfDirDialog()
{
    const FilePath dir = FileUtils::getExistingDirectory(
        Tr::tr("Load Common Trace Format Directory"), settings().lastTraceFile().parentDir());

    if (!dir.isEmpty())
        q->loadTraceFile(dir);
}

void WindowPrivate::selectBackend(int index)
{
    if (index < 0 || index >= int(backends.size()))
        return;
    sampler = backends[index].get();
    QWidget *config = nullptr;
    if (SamplerSettings *s = sampler->settings()) {
        config = new QWidget;
        s->layouter()().attachTo(config);
    }
    welcomePage->setActiveBackend(config);
}

bool WindowPrivate::selectBackendByName(const QString &name)
{
    for (int i = 0; i < int(backends.size()); ++i) {
        if (backends[i]->displayName().contains(name, Qt::CaseInsensitive)) {
            welcomePage->setCurrentBackend(i); // emits backendChanged() -> selectBackend()
            return true;
        }
    }
    return false;
}

void WindowPrivate::startTimedRecording(milliseconds duration)
{
    startRecording();
    if (!recording)
        return; // startRecording() reported the error already.

    // The clock only starts once the backend is actually capturing (after launch
    // and, for the QML profiler, after the debug connection is up); otherwise the
    // launch/connect delay would eat into the requested span. The processing poll
    // watches session->started and arms the stop timer on the 0->1 transition.
    recordDuration = duration;
    recordDurationArmed = false;
}

void WindowPrivate::startRecording()
{
    if (recording)
        return;

    QString reason;
    if (!sampler->isAvailable(&reason)) {
        onError(reason);
        return;
    }

    SamplerSettings *settings = sampler->settings();
    if (!settings) {
        onError(Tr::tr("This backend cannot be configured for recording."));
        return;
    }

    const Result<std::shared_ptr<RecordingSession>> created = settings->createSession();
    if (!created) {
        onError(created.error());
        return;
    }
    session = *created;

    // A name for the recording page: the launched command, the attach target, or
    // the connect endpoint.
    QString name;
    if (session->launchCommand)
        name = session->launchCommand->executable().fileName();
    else if (!session->processName.isEmpty())
        name = session->processName;
    else if (!session->serverUrl.isEmpty())
        name = session->serverUrl.host() + u':' + QString::number(session->serverUrl.port());
    beginRecording(name);
}

void WindowPrivate::beginRecording(const QString &displayName)
{
    recording = true;
    processingShown = false;
    recordingPage->start(displayName);
    rightPane->setCurrentWidget(recordingPage);
    processingPoll->start();

    // The backend owns the complete recipe: it launches the target (when
    // session->launchCommand is set), captures it, and tears it down. When the
    // tree is done, finishRecording() loads the captured trace.
    recordingRunner.start(QtTaskTree::Group{sampler->recordRecipe(session)}, [] {},
                          [this](QtTaskTree::DoneWith) { finishRecording(); });
}

void WindowPrivate::finishRecording()
{
    recording = false;
    processingPoll->stop();
    recordingPage->stop();
    recordDuration.reset(); // one-shot: don't auto-stop a later manual recording
    recordDurationArmed = false;

    const std::shared_ptr<RecordingSession> finished = session;
    session.reset();

    if (closing)
        return; // The window is shutting down: don't touch the UI or load a trace.

    if (!finished || !finished->result) {
        rightPane->setCurrentWidget(welcomePage);
        onError(Tr::tr("Recording did not produce a trace."));
        return;
    }
    const Result<FilePath> &result = *finished->result;
    if (!result) {
        rightPane->setCurrentWidget(welcomePage);
        onError(result.error());
        return;
    }
    q->loadTraceFile(*result);
}

void WindowPrivate::stopAndWaitForRecording()
{
    if (!recording || !session)
        return;
    // The sampler loops until stopped; ask it to finish and pump events until the
    // task tree (and its worker thread) have wound down. Otherwise the global
    // thread pool would block at exit and the launched process would be left
    // running.
    closing = true;
    session->stop.store(true);
    while (recording)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
}

void WindowPrivate::onError(const QString &error)
{
    lastLoadError = error;
    progressIndicator->hide();

    if (settings().exitOnError()) {
        std::cerr << lastLoadError.toStdString() << std::endl;
        // Give time for further error notifications.
        QTimer::singleShot(0, []{ QApplication::exit(1); });
    }

    QMessageBox::critical(nullptr, Tr::tr("Error"), error);
}

void WindowPrivate::onLoadFinished()
{
    setTraceDuration(activeTraceDuration());
    progressIndicator->hide();
    RPC::notifyTraceFileLoadingFinished(settings().lastTraceFile(), lastLoadError);
}

void WindowPrivate::onGotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber,
                                         const QString &module, quint64 offset)
{
    RPC::notifyTraceEventSelected(fileUrl, lineNumber, columnNumber, module, offset);
}

milliseconds WindowPrivate::activeTraceDuration() const
{
    switch (activeFormat) {
    case Format::Qml: return qmlManager->traceDuration();
    case Format::Ctf: return ctfManager->traceDuration();
    case Format::Sampler: return samplerManager->traceDuration();
    }
    Q_UNREACHABLE_RETURN(milliseconds{0});
}

void WindowPrivate::addDocksForViews()
{
    QTC_ASSERT(qmlGroup.docks.isEmpty() && ctfGroup.docks.isEmpty()
                   && samplerGroup.docks.isEmpty(),
               return);

    const auto buildGroup = [this](const QWidgetList &views, QWidget *details, ViewGroup &group) {
        for (QWidget *w : views)
            group.docks.append(traceArea->addDockForWidget(w));
        // The range details view lives next to the tabbed views, docked to the right.
        if (details)
            group.detailsDock = traceArea->addDockForWidget(details);
    };

    // views() populates the manager's range-details widget, so query it afterwards.
    const QWidgetList qmlViews = qmlManager->views(traceArea);
    buildGroup(qmlViews, qmlManager->rangeDetailsWidget(), qmlGroup);

    const QWidgetList ctfViews = ctfManager->views(traceArea);
    buildGroup(ctfViews, ctfManager->rangeDetailsWidget(), ctfGroup);

    const QWidgetList samplerViews = samplerManager->views(traceArea);
    buildGroup(samplerViews, samplerManager->rangeDetailsWidget(), samplerGroup);
    // Stack Call Stacks below CPU Usage instead of tabbing them together.
    samplerGroup.stackVertically = true;
}

void WindowPrivate::resetLayout(Format format)
{
    const auto groupFor = [this](Format f) -> ViewGroup & {
        switch (f) {
        case Format::Qml: return qmlGroup;
        case Format::Ctf: return ctfGroup;
        case Format::Sampler: return samplerGroup;
        }
        Q_UNREACHABLE_RETURN(qmlGroup);
    };
    ViewGroup &active = groupFor(format);

    // Hide the docks belonging to the other formats.
    for (Format other : {Format::Qml, Format::Ctf, Format::Sampler}) {
        if (other == format)
            continue;
        ViewGroup &inactive = groupFor(other);
        for (QDockWidget *dw : std::as_const(inactive.docks))
            dw->setVisible(false);
        if (inactive.detailsDock)
            inactive.detailsDock->setVisible(false);
    }

    QDockWidget *firstDockWidget = nullptr;
    for (QDockWidget *dw : std::as_const(active.docks)) {
        dw->setVisible(true);
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        dw->setDockLocation(Qt::TopDockWidgetArea);
#endif
        dw->setFloating(false);
        if (!firstDockWidget)
            firstDockWidget = dw;
    }
    QTC_ASSERT(firstDockWidget, return);

    // Split the details off the first view before tabbing the others onto it.
    // QMainWindow::splitDockWidget() only splits when the anchor is not yet tabbed;
    // doing this after tabifying would just add the details as another tab.
    if (active.detailsDock) {
        active.detailsDock->setVisible(true);
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        active.detailsDock->setDockLocation(Qt::RightDockWidgetArea);
#endif
        active.detailsDock->setFloating(false);
        traceArea->splitDockWidget(firstDockWidget, active.detailsDock, Qt::Horizontal);
    }

    for (QDockWidget *dw : std::as_const(active.docks)) {
        if (dw == firstDockWidget)
            continue;
        if (active.stackVertically)
            traceArea->splitDockWidget(firstDockWidget, dw, Qt::Vertical);
        else
            traceArea->tabifyDockWidget(firstDockWidget, dw);
    }
    firstDockWidget->raise();
}

void WindowPrivate::setActiveFormat(Format format)
{
    if (activeFormat == format)
        return;
    activeFormat = format;
    resetLayout(format);
}

void WindowPrivate::clearTrace()
{
    // Removing the current trace selects a neighbour, which reloads it via
    // traceActivated(). Only when nothing remains do we clear and show the
    // welcome page.
    if (sidebar->removeCurrentTrace())
        return;

    setTraceDuration(milliseconds{0});
    qmlManager->clear();
    ctfManager->clear();
    samplerManager->clear();
    rightPane->setCurrentWidget(welcomePage);
    RPC::notifyTraceDiscarded();
}

void WindowPrivate::doLoad(const FilePath &filePath)
{
    settings().lastTraceFile.setValue(filePath);
    rightPane->setCurrentWidget(traceArea);
    setTraceDuration(milliseconds{0});
    progressIndicator->show();
    lastLoadError.clear();
    RPC::notifyTraceFileLoadingStarted(filePath);

    // Pick the manager by file format. A directory or a bare "metadata" file
    // holds either a recorded sampler trace or a generic Common Trace Format
    // trace; a .json file is Chrome Trace Format; anything else is treated as
    // a QML profiler trace.
    if (filePath.isDir() || filePath.fileName() == "metadata"_L1) {
        const FilePath dir = filePath.isDir() ? filePath : filePath.parentDir();
        if (SamplerViewManager::isSamplerTrace(dir)) {
            setActiveFormat(Format::Sampler);
            samplerManager->load(dir);
        } else {
            setActiveFormat(Format::Ctf);
            ctfManager->loadCtf2(dir);
        }
    } else if (filePath.suffix() == "json"_L1) {
        setActiveFormat(Format::Ctf);
        ctfManager->loadJson(filePath);
    } else {
        setActiveFormat(Format::Qml);
        qmlManager->loadTraceFile(filePath);
    }
}

void WindowPrivate::setTraceDuration(milliseconds ms)
{
    const int msCount = ms.count();
    const QTime durationTime = QTime::fromMSecsSinceStartOfDay(msCount);
    const QString format = msCount >= 60 * 60 * 1000
                               ? "H:mm:ss' h'"_L1
                               : msCount >= 60 * 1000 ? "m:ss' m'"_L1
                                                      : "s.z' s'"_L1;
    const QString durationString = durationTime.toString(format);
    const QString text = Tr::tr("Trace duration: %1").arg(durationString);
    traceDurationLabel->setText(text);
}

void WindowPrivate::openHelpInBrowser()
{
    QDesktopServices::openUrl({"https://doc.qt.io/qtcreator/creator-qml-performance-monitor.html"});
}

Window::Window(QWidget *parent)
    : QMainWindow(parent)
    , d(new WindowPrivate(this))
{
    auto loadAction = new QAction(Icons::OPENFILE_TOOLBAR.icon(), Tr::tr("Load Trace"), this);
    loadAction->setToolTip(Tr::tr("Load a QML or Chrome Trace Format trace file."));
    loadAction->setShortcut(QKeySequence::Open);
    connect(loadAction, &QAction::triggered, d, &WindowPrivate::showOpenFileDialog);

    auto loadCtfDirAction
        = new QAction(Icons::DIR.icon(), Tr::tr("Load Common Trace Format Directory"), this);
    loadCtfDirAction->setToolTip(Tr::tr("Load a Common Trace Format trace directory."));
    connect(loadCtfDirAction, &QAction::triggered, d, &WindowPrivate::showOpenCtfDirDialog);

    auto clearAction = new QAction(Icons::CLEAN_TOOLBAR.icon(), Tr::tr("Discard Data"), this);
    clearAction->setShortcut(QKeySequence::Delete);
    connect(clearAction, &QAction::triggered, d, &WindowPrivate::clearTrace);

    auto helpAction = new QAction("?", this);
    helpAction->setToolTip(Tr::tr("Open Help in Web Browser"));
    helpAction->setShortcut(QKeySequence::HelpContents);
    connect(helpAction, &QAction::triggered, this, &WindowPrivate::openHelpInBrowser);

    d->traceDurationLabel = new QLabel;
    d->traceDurationLabel->setContentsMargins(SpacingTokens::PaddingHM, 0,
                                              SpacingTokens::PaddingHM, 0);
    d->traceDurationLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    auto toolBar = new QToolBar;
    toolBar->setObjectName("QmlProfileTraceViewer");
    toolBar->addActions(QList<QAction *>{loadAction, loadCtfDirAction, clearAction});
    toolBar->addSeparator();
    toolBar->addWidget(d->traceDurationLabel);
    toolBar->addAction(helpAction);
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(16, 16));
    addToolBar(toolBar);

    d->addDocksForViews();
    d->resetActiveLayout();
    connect(d->traceArea, &FancyMainWindow::resetLayout, d, &WindowPrivate::resetActiveLayout);

    auto splitter = new Core::MiniSplitter(Qt::Horizontal);
    splitter->addWidget(d->sidebar);
    splitter->addWidget(d->rightPane);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({200, 800});
    setCentralWidget(splitter);

    d->progressIndicator->attachToWidget(this);

    restoreGeometry(settings().windowGeometry());
}

void Window::loadTraceFile(const FilePath &filePath)
{
    d->sidebar->addTrace(filePath); // Registers + selects it without triggering a reload.
    d->doLoad(filePath);
}

bool Window::selectBackend(const QString &name)
{
    return d->selectBackendByName(name);
}

void Window::startTimedRecording(std::chrono::milliseconds duration)
{
    d->startTimedRecording(duration);
}

void Window::closeEvent(QCloseEvent *event)
{
    settings().windowGeometry.setValue(saveGeometry());
    for (const std::unique_ptr<Sampler> &backend : d->backends) {
        if (Utils::AspectContainer *s = backend->settings())
            s->writeSettings();
    }
    d->stopAndWaitForRecording();
    QMainWindow::closeEvent(event);
}

} // namespace QtProfiler
