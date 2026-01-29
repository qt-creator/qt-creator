// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltraceviewerwindow.h"

#include "qmltraceviewersettings.h"

#include <qmlprofiler/qmlprofilerconstants.h>
#include <qmlprofiler/qmlprofilerplainviewmanager.h>
#include <qmlprofiler/qmlprofilertr.h>

#include <utils/fileutils.h>
#include <utils/progressindicator.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QDockWidget>
#include <QMessageBox>
#include <QTime>
#include <QToolBar>
#include <QToolButton>

#include <iostream>

using namespace QmlProfiler;
using namespace Utils;
using namespace Utils::StyleHelper;

using namespace Qt::StringLiterals;

using namespace std::chrono;

namespace QmlTraceViewer {

class WindowPrivate : public QObject
{
public:
    WindowPrivate(Window *window = nullptr);

    QFuture<void> showOpenFileDialog();

    void onError(const QString &error);
    void onLoadFinished();
    void onGotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber);

    void addDocksForViews();
    void resetLayout();
    void clearTrace();
    void setTraceDuration(milliseconds ms);

    Window *q = nullptr;
    QmlProfilerPlainViewManager *viewManager;
    QLabel *traceDurationLabel;
    ProgressIndicator *progressIndicator;
    QList<QDockWidget*> dockWidgets; // List with original dock widgets order
};

WindowPrivate::WindowPrivate(Window *window)
    : QObject(window)
    , q(window)
{
    viewManager = new QmlProfilerPlainViewManager(q);

    progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Large);
    progressIndicator->hide();

    connect(viewManager, &QmlProfilerPlainViewManager::error, this, &WindowPrivate::onError);
    connect(viewManager, &QmlProfilerPlainViewManager::loadFinished,
            this, &WindowPrivate::onLoadFinished);
    connect(viewManager, &QmlProfilerPlainViewManager::gotoSourceLocation,
            this, &WindowPrivate::onGotoSourceLocation);
}

QFuture<void> WindowPrivate::showOpenFileDialog()
{
    const FilePath filePath = FileUtils::getOpenFilePath(
        Tr::tr("Load QML Trace"),
        settings().lastTraceFile(),
        QmlProfilerPlainViewManager::fileDialogTraceFilesFilter());

    if (!filePath.isEmpty()) {
        settings().lastTraceFile.setValue(filePath);
        return q->loadTraceFile(filePath);
    }

    return {};
}

void WindowPrivate::onError(const QString &error)
{
    progressIndicator->hide();
    if (settings().exitOnError()) {
        std::cerr << error.toStdString() << std::endl;
        QApplication::exit(1);
    } else {
        QMessageBox::critical(nullptr, Tr::tr("Error"), error);
    }
}

void WindowPrivate::onLoadFinished()
{
    setTraceDuration(viewManager->traceDuration());
    progressIndicator->hide();
}

void WindowPrivate::onGotoSourceLocation(const QString &fileUrl, int lineNumber,
                                         [[maybe_unused]] int columnNumber)
{
    if (!settings().printSourceLocations())
        return;
    const QString location = "%1:%2"_L1.arg(fileUrl).arg(lineNumber);
    std::cout << location.toStdString() << std::endl;
}

void WindowPrivate::addDocksForViews()
{
    QTC_ASSERT(dockWidgets.isEmpty(), return);
    const QWidgetList views = viewManager->views(q);
    for (QWidget *w : views)
        dockWidgets.append(q->addDockForWidget(w));
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
        else
            q->tabifyDockWidget(firstDockWidget, dw);
    }
    firstDockWidget->raise();
}

void WindowPrivate::clearTrace()
{
    setTraceDuration(milliseconds{0});
    viewManager->clear();
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

Window::Window(QWidget *parent)
    : FancyMainWindow(parent)
    , d(new WindowPrivate(this))
{
    setDockNestingEnabled(true);
    setDocumentMode(true);

    auto loadAction = new QAction(Icons::OPENFILE_TOOLBAR.icon(), Tr::tr("Load QML Trace"), this);
    loadAction->setShortcut(QKeySequence::Open);
    connect(loadAction, &QAction::triggered, d, &WindowPrivate::showOpenFileDialog);

    auto clearAction = new QAction(Icons::CLEAN_TOOLBAR.icon(), Tr::tr("Discard Data"), this);
    clearAction->setShortcut(QKeySequence::Delete);
    connect(clearAction, &QAction::triggered, d, &WindowPrivate::clearTrace);

    d->traceDurationLabel = new QLabel;
    d->traceDurationLabel->setContentsMargins(SpacingTokens::PaddingHM, 0,
                                              SpacingTokens::PaddingHM, 0);

    auto toolBar = new QToolBar;
    toolBar->setObjectName("QmlProfileTraceViewer");
    toolBar->addActions({loadAction, clearAction});
    toolBar->addSeparator();
    toolBar->addWidget(d->traceDurationLabel);
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(16, 16));
    addToolBar(toolBar);

    d->addDocksForViews();
    d->resetLayout();
    connect(this, &FancyMainWindow::resetLayout, d, &WindowPrivate::resetLayout);

    d->progressIndicator->attachToWidget(this);

    restoreGeometry(settings().windowGeometry());
}

QFuture<void> Window::loadTraceFile(const FilePath &file)
{
    d->setTraceDuration(milliseconds{0});
    d->progressIndicator->show();
    return d->viewManager->loadTraceFile(file);
}

void Window::closeEvent(QCloseEvent *event)
{
    settings().windowGeometry.setValue(saveGeometry());
    QMainWindow::closeEvent(event);
}

} // namespace QmlTraceViewer
