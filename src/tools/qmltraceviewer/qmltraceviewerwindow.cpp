// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltraceviewerwindow.h"

#include <profiler/macsampler.h>
#include "qmltraceviewerrpc.h"
#include "qmltraceviewersettings.h"
#include "mainsidebar.h"
#include "recordingpage.h"
#include "welcomepage.h"

#include <profiler/ctfplainviewmanager.h>
#include <profiler/profilertr.h>
#include <profiler/qmlprofilerconstants.h>
#include <profiler/qmlprofilerplainviewmanager.h>

#include <coreplugin/minisplitter.h>

#include <utils/fancymainwindow.h>
#include <utils/fileutils.h>
#include <utils/progressindicator.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTime>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>

#include <QtConcurrent>

#include <atomic>
#include <iostream>
#include <memory>

using namespace Profiler;
using namespace QmlProfiler::Internal;
using namespace Utils;
using namespace Utils::StyleHelper;

using namespace Qt::StringLiterals;

using namespace std::chrono;

namespace QmlTraceViewer {

// A trace file format determines which manager and view set are used. Chrome
// Trace Format and Common Trace Format both render through the CTF views.
enum class Format { Qml, Ctf };

// One tabbed set of dock widgets, plus its (untabbed) range-details dock.
struct ViewGroup
{
    QList<QDockWidget *> docks;         // List with original dock widgets order
    QDockWidget *detailsDock = nullptr; // Range details, docked to the right (not tabbed)
};

class WindowPrivate : public QObject
{
public:
    WindowPrivate(Window *window = nullptr);

    void showOpenFileDialog();
    void showOpenCtfDirDialog();
    void startRecording();

    void onError(const QString &error);
    void onLoadFinished();
    void onGotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber);

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
    Format activeFormat = Format::Qml;
    QString lastLoadError;
    QLabel *traceDurationLabel = nullptr;
    ProgressIndicator *progressIndicator;
    ViewGroup qmlGroup;
    ViewGroup ctfGroup;
    bool recording = false;
    std::shared_ptr<std::atomic_bool> stopRecording;
    std::shared_ptr<std::atomic<int>> recordProgress;
    QTimer *processingPoll = nullptr;
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

    connect(welcomePage, &WelcomePage::openTraceRequested,
            this, &WindowPrivate::showOpenFileDialog);
    connect(welcomePage, &WelcomePage::recordTraceRequested,
            this, &WindowPrivate::startRecording);
    processingPoll = new QTimer(this);
    processingPoll->setInterval(50);
    connect(processingPoll, &QTimer::timeout, this, [this] {
        if (recordProgress)
            recordingPage->setProgress(recordProgress->load(std::memory_order_relaxed));
    });

    connect(recordingPage, &RecordingPage::stopRequested, this, [this] {
        if (stopRecording)
            stopRecording->store(true);
        recordingPage->setProcessing();
        processingPoll->start();
    });

    qmlManager = new QmlProfilerPlainViewManager(traceArea);
    ctfManager = new CtfPlainViewManager(traceArea);

    progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Large);
    progressIndicator->hide();

    connect(sidebar, &MainSidebar::traceActivated, this, &WindowPrivate::doLoad);

    connect(qmlManager, &QmlProfilerPlainViewManager::error, this, &WindowPrivate::onError);
    connect(qmlManager, &QmlProfilerPlainViewManager::loadFinished,
            this, &WindowPrivate::onLoadFinished);
    connect(qmlManager, &QmlProfilerPlainViewManager::gotoSourceLocation,
            this, &WindowPrivate::onGotoSourceLocation);

    connect(ctfManager, &CtfPlainViewManager::error, this, &WindowPrivate::onError);
    connect(ctfManager, &CtfPlainViewManager::loadFinished, this, &WindowPrivate::onLoadFinished);
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

void WindowPrivate::startRecording()
{
    if (recording)
        return;
    recording = true;

    SamplerOptions opts;
    opts.processName = settings().recordProcessName();
    opts.intervalUs = int(settings().recordIntervalUs());

    // Sampling suspends/walks the target in a tight loop, so it must run off the
    // GUI thread. It samples until `stopRecording` is set by the Stop button,
    // then publishes post-processing progress through `recordProgress`.
    stopRecording = std::make_shared<std::atomic_bool>(false);
    recordProgress = std::make_shared<std::atomic<int>>(0);
    recordingPage->start(opts.processName);
    rightPane->setCurrentWidget(recordingPage);

    auto watcher = new QFutureWatcher<Result<FilePath>>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher] {
        watcher->deleteLater();
        recording = false;
        processingPoll->stop();
        stopRecording.reset();
        recordProgress.reset();
        recordingPage->stop();

        const Result<FilePath> result = watcher->result();
        if (!result) {
            rightPane->setCurrentWidget(welcomePage);
            onError(result.error());
            return;
        }
        q->loadTraceFile(*result);
    });
    const std::shared_ptr<std::atomic_bool> stop = stopRecording;
    const std::shared_ptr<std::atomic<int>> progress = recordProgress;
    watcher->setFuture(QtConcurrent::run([opts, stop, progress] {
        return recordSampleTrace(opts, *stop, progress.get());
    }));
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

void WindowPrivate::onGotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber)
{
    RPC::notifyTraceEventSelected(fileUrl, lineNumber, columnNumber);
}

milliseconds WindowPrivate::activeTraceDuration() const
{
    return activeFormat == Format::Qml ? qmlManager->traceDuration() : ctfManager->traceDuration();
}

void WindowPrivate::addDocksForViews()
{
    QTC_ASSERT(qmlGroup.docks.isEmpty() && ctfGroup.docks.isEmpty(), return);

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
}

void WindowPrivate::resetLayout(Format format)
{
    ViewGroup &active = format == Format::Qml ? qmlGroup : ctfGroup;
    ViewGroup &inactive = format == Format::Qml ? ctfGroup : qmlGroup;

    // Hide the docks belonging to the other format.
    for (QDockWidget *dw : std::as_const(inactive.docks))
        dw->setVisible(false);
    if (inactive.detailsDock)
        inactive.detailsDock->setVisible(false);

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
        if (dw != firstDockWidget)
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

    // Pick the manager by file format. A directory or a bare "metadata" file is a
    // Common Trace Format trace; a .json file is Chrome Trace Format; anything
    // else is treated as a QML profiler trace.
    if (filePath.isDir()) {
        setActiveFormat(Format::Ctf);
        ctfManager->loadCtf2(filePath);
    } else if (filePath.fileName() == "metadata"_L1) {
        setActiveFormat(Format::Ctf);
        ctfManager->loadCtf2(filePath.parentDir());
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

void Window::closeEvent(QCloseEvent *event)
{
    settings().windowGeometry.setValue(saveGeometry());
    QMainWindow::closeEvent(event);
}

} // namespace QmlTraceViewer
