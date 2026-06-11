// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltraceviewerwindow.h"

#include "qmltraceviewerrpc.h"
#include "qmltraceviewersettings.h"
#include "mainsidebar.h"

#include <qmlprofiler/profilertr.h>
#include <qmlprofiler/qmlprofilerconstants.h>
#include <qmlprofiler/qmlprofilerplainviewmanager.h>

#include <coreplugin/minisplitter.h>

#include <utils/fancymainwindow.h>
#include <utils/fileutils.h>
#include <utils/progressindicator.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDockWidget>
#include <QMessageBox>
#include <QSplitter>
#include <QTime>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>

#include <iostream>

using namespace Profiler;
using namespace Utils;
using namespace Utils::StyleHelper;

using namespace Qt::StringLiterals;

using namespace std::chrono;

namespace QmlTraceViewer {

class WindowPrivate : public QObject
{
public:
    WindowPrivate(Window *window = nullptr);

    void showOpenFileDialog();

    void onError(const QString &error);
    void onLoadFinished();
    void onGotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber);

    void addDocksForViews();
    void resetLayout();
    void clearTrace();
    void doLoad(const Utils::FilePath &filePath);
    void setTraceDuration(milliseconds ms);

    static void openHelpInBrowser();

    Window *q = nullptr;
    FancyMainWindow *traceArea = nullptr; // Hosts the trace view docks (right pane).
    MainSidebar *sidebar = nullptr; // List of opened traces (left pane).
    QmlProfilerPlainViewManager *viewManager;
    QString lastLoadError;
    QLabel *traceDurationLabel = nullptr;
    ProgressIndicator *progressIndicator;
    QList<QDockWidget*> dockWidgets; // List with original dock widgets order
    QDockWidget *detailsDock = nullptr; // Range details, docked to the right (not tabbed)
};

WindowPrivate::WindowPrivate(Window *window)
    : QObject(window)
    , q(window)
{
    traceArea = new FancyMainWindow(q);
    traceArea->setDockNestingEnabled(true);
    traceArea->setDocumentMode(true);

    sidebar = new MainSidebar(q);

    viewManager = new QmlProfilerPlainViewManager(traceArea);

    progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Large);
    progressIndicator->hide();

    connect(sidebar, &MainSidebar::traceActivated, this, &WindowPrivate::doLoad);

    connect(viewManager, &QmlProfilerPlainViewManager::error, this, &WindowPrivate::onError);
    connect(viewManager, &QmlProfilerPlainViewManager::loadFinished,
            this, &WindowPrivate::onLoadFinished);
    connect(viewManager, &QmlProfilerPlainViewManager::gotoSourceLocation,
            this, &WindowPrivate::onGotoSourceLocation);
}

void WindowPrivate::showOpenFileDialog()
{
    const FilePath filePath = FileUtils::getOpenFilePath(
        Tr::tr("Load QML Trace"),
        settings().lastTraceFile(),
        QmlProfilerPlainViewManager::fileDialogTraceFilesFilter());

    if (!filePath.isEmpty())
        q->loadTraceFile(filePath);
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
    setTraceDuration(viewManager->traceDuration());
    progressIndicator->hide();
    RPC::notifyTraceFileLoadingFinished(settings().lastTraceFile(), lastLoadError);
}

void WindowPrivate::onGotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber)
{
    RPC::notifyTraceEventSelected(fileUrl, lineNumber, columnNumber);
}

void WindowPrivate::addDocksForViews()
{
    QTC_ASSERT(dockWidgets.isEmpty(), return);
    const QWidgetList views = viewManager->views(traceArea);
    for (QWidget *w : views)
        dockWidgets.append(traceArea->addDockForWidget(w));

    // The range details view lives next to the tabbed views, docked to the right.
    if (QWidget *details = viewManager->rangeDetailsWidget())
        detailsDock = traceArea->addDockForWidget(details);
}

void WindowPrivate::resetLayout()
{
    QDockWidget *firstDockWidget = nullptr;
    for (QDockWidget *dw : std::as_const(dockWidgets)) {
        dw->setVisible(true);
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        dw->setDockLocation(Qt::TopDockWidgetArea);
#endif
        dw->setFloating(false);
        if (!firstDockWidget)
            firstDockWidget = dw;
    }

    // Split the details off the first view before tabbing the others onto it.
    // QMainWindow::splitDockWidget() only splits when the anchor is not yet tabbed;
    // doing this after tabifying would just add the details as another tab.
    if (detailsDock) {
        detailsDock->setVisible(true);
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        detailsDock->setDockLocation(Qt::RightDockWidgetArea);
#endif
        detailsDock->setFloating(false);
        traceArea->splitDockWidget(firstDockWidget, detailsDock, Qt::Horizontal);
    }

    for (QDockWidget *dw : std::as_const(dockWidgets)) {
        if (dw != firstDockWidget)
            traceArea->tabifyDockWidget(firstDockWidget, dw);
    }
    firstDockWidget->raise();
}

void WindowPrivate::clearTrace()
{
    setTraceDuration(milliseconds{0});
    viewManager->clear();
    RPC::notifyTraceDiscarded();
}

void WindowPrivate::doLoad(const FilePath &filePath)
{
    settings().lastTraceFile.setValue(filePath);
    setTraceDuration(milliseconds{0});
    progressIndicator->show();
    lastLoadError.clear();
    RPC::notifyTraceFileLoadingStarted(filePath);
    viewManager->loadTraceFile(filePath);
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
    auto loadAction = new QAction(Icons::OPENFILE_TOOLBAR.icon(), Tr::tr("Load QML Trace"), this);
    loadAction->setShortcut(QKeySequence::Open);
    connect(loadAction, &QAction::triggered, d, &WindowPrivate::showOpenFileDialog);

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
    toolBar->addActions(QList<QAction *>{loadAction, clearAction});
    toolBar->addSeparator();
    toolBar->addWidget(d->traceDurationLabel);
    toolBar->addAction(helpAction);
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(16, 16));
    addToolBar(toolBar);

    d->addDocksForViews();
    d->resetLayout();
    connect(d->traceArea, &FancyMainWindow::resetLayout, d, &WindowPrivate::resetLayout);

    auto splitter = new Core::MiniSplitter(Qt::Horizontal);
    splitter->addWidget(d->sidebar);
    splitter->addWidget(d->traceArea);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({200, 800});
    setCentralWidget(splitter);

    d->progressIndicator->attachToWidget(d->traceArea);

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
