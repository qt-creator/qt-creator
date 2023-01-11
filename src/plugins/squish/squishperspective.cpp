// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishperspective.h"

#include "squishtools.h"
#include "squishtr.h"
#include "squishxmloutputhandler.h"

#include <debugger/analyzer/analyzermanager.h>
#include <debugger/debuggericons.h>
#include <coreplugin/icore.h>

#include <utils/itemviews.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QScreen>
#include <QToolBar>
#include <QVBoxLayout>

namespace Squish {
namespace Internal {

enum class IconType { StopRecord, Play, Pause, StepIn, StepOver, StepReturn, Stop };

static QIcon iconForType(IconType type)
{
    switch (type) {
    case IconType::StopRecord:
        return Debugger::Icons::RECORD_ON.icon();
    case IconType::Play:
        return Debugger::Icons::DEBUG_CONTINUE_SMALL_TOOLBAR.icon();
    case IconType::Pause:
        return Utils::Icons::INTERRUPT_SMALL.icon();
    case IconType::StepIn:
        return Debugger::Icons::STEP_INTO_TOOLBAR.icon();
    case IconType::StepOver:
        return Debugger::Icons::STEP_OVER_TOOLBAR.icon();
    case IconType::StepReturn:
        return Debugger::Icons::STEP_OUT_TOOLBAR.icon();
    case IconType::Stop:
        return Utils::Icons::STOP_SMALL.icon();
    }
    return QIcon();
}


static QString customStyleSheet(bool extended)
{
    static const QString red = Utils::creatorTheme()->color(
                Utils::Theme::ProgressBarColorError).name();
    static const QString green = Utils::creatorTheme()->color(
                Utils::Theme::ProgressBarColorFinished).name();
    if (!extended)
        return "QProgressBar {text-align:left; border:0px}";
    return QString("QProgressBar {background:%1; text-align:left; border:0px}"
                   "QProgressBar::chunk {background:%2; border:0px}").arg(red, green);
}

static QStringList splitDebugContent(const QString &content)
{
    if (content.isEmpty())
        return {};
    QStringList symbols;
    int delimiter = -1;
    int start = 0;
    do {
        delimiter = content.indexOf(',', delimiter + 1);
        if (delimiter > 0 && content.at(delimiter - 1) == '\\')
            continue;
        symbols.append(content.mid(start, delimiter - start));
        start = delimiter + 1;
    } while (delimiter >= 0);
    return symbols;
}

QVariant LocalsItem::data(int column, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (column) {
        case 0: return name;
        case 1: return type;
        case 2: return value;
        }
    }
    return TreeItem::data(column, role);
}

class SquishControlBar : public QDialog
{
public:
    explicit SquishControlBar(SquishPerspective *perspective);

    void increaseFailCounter() { ++m_fails; updateProgressBar(); }
    void increasePassCounter() { ++m_passes; updateProgressBar(); }
    void updateProgressText(const QString &label);

protected:
    void closeEvent(QCloseEvent *) override
    {
        m_perspective->onStopTriggered();
    }
    void resizeEvent(QResizeEvent *) override
    {
        updateProgressText(m_labelText);
    }

private:
    void updateProgressBar();

    SquishPerspective *m_perspective = nullptr;
    QToolBar *m_toolBar = nullptr;
    QProgressBar *m_progress = nullptr;
    QString m_labelText;
    int m_passes = 0;
    int m_fails = 0;
};

SquishControlBar::SquishControlBar(SquishPerspective *perspective)
    : QDialog()
    , m_perspective(perspective)
{
    setWindowTitle(Tr::tr("Control Bar"));
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_toolBar = new QToolBar(this));
    // for now
    m_toolBar->addAction(perspective->m_stopRecordAction);
    m_toolBar->addAction(perspective->m_pausePlayAction);
    m_toolBar->addAction(perspective->m_stopAction);

    mainLayout->addWidget(m_progress = new QProgressBar(this));
    m_progress->setMinimumHeight(48);
    m_progress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    m_progress->setStyleSheet(customStyleSheet(false));
    m_progress->setFormat(QString());
    m_progress->setValue(0);
    QPalette palette = Utils::creatorTheme()->palette();
    m_progress->setPalette(palette);

    setLayout(mainLayout);
}

void SquishControlBar::updateProgressBar()
{
    const int allCounted = m_passes + m_fails;
    if (allCounted == 0)
        return;
    if (allCounted == 1) {
        QPalette palette = m_progress->palette();
        palette.setColor(QPalette::Text, Qt::black);
        m_progress->setStyleSheet(customStyleSheet(true));
        m_progress->setPalette(palette);
    }

    m_progress->setRange(0, allCounted);
    m_progress->setValue(m_passes);
}

void SquishControlBar::updateProgressText(const QString &label)
{
    const QString status = m_progress->fontMetrics().elidedText(label, Qt::ElideMiddle,
                                                                m_progress->width());
    if (!status.isEmpty()) {
        m_labelText = label;
        m_progress->setFormat(status);
    }
}

SquishPerspective::SquishPerspective()
    : Utils::Perspective("Squish.Perspective", Tr::tr("Squish"))
{
    Core::ICore::addPreCloseListener([this]{
        destroyControlBar();
        return true;
    });
}

void SquishPerspective::initPerspective()
{
    m_stopRecordAction = new QAction(this);
    m_stopRecordAction->setIcon(iconForType(IconType::StopRecord));
    m_stopRecordAction->setToolTip(Tr::tr("Stop Recording") + "\n\n"
                                   + Tr::tr("Ends the recording session, "
                                            "saving all commands to the script file."));
    m_stopRecordAction->setEnabled(false);
    m_pausePlayAction = new QAction(this);
    m_pausePlayAction->setIcon(iconForType(IconType::Pause));
    m_pausePlayAction->setToolTip(Tr::tr("Interrupt"));
    m_pausePlayAction->setEnabled(false);
    m_stepInAction = new QAction(this);
    m_stepInAction->setIcon(iconForType(IconType::StepIn));
    m_stepInAction->setToolTip(Tr::tr("Step Into"));
    m_stepInAction->setEnabled(false);
    m_stepOverAction = new QAction(this);
    m_stepOverAction->setIcon(iconForType(IconType::StepOver));
    m_stepOverAction->setToolTip(Tr::tr("Step Over"));
    m_stepOverAction->setEnabled(false);
    m_stepOutAction = new QAction(this);
    m_stepOutAction->setIcon(iconForType(IconType::StepReturn));
    m_stepOutAction->setToolTip(Tr::tr("Step Out"));
    m_stepOutAction->setEnabled(false);
    m_stopAction = Debugger::createStopAction();
    m_stopAction->setEnabled(false);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(1);

    m_localsModel.setHeader({Tr::tr("Name"), Tr::tr("Type"), Tr::tr("Value")});
    auto localsView = new Utils::TreeView;
    localsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    localsView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    localsView->setModel(&m_localsModel);
    localsView->setRootIsDecorated(true);
    mainLayout->addWidget(localsView);
    QWidget *mainWidget = new QWidget;
    mainWidget->setObjectName("SquishLocalsView");
    mainWidget->setWindowTitle(Tr::tr("Squish Locals"));
    mainWidget->setLayout(mainLayout);

    addToolBarAction(m_pausePlayAction);
    addToolBarAction(m_stepInAction);
    addToolBarAction(m_stepOverAction);
    addToolBarAction(m_stepOutAction);
    addToolBarAction(m_stopAction);
    addToolbarSeparator();
    m_status = new QLabel;
    addToolBarWidget(m_status);

    addWindow(mainWidget, Perspective::AddToTab, nullptr, true, Qt::RightDockWidgetArea);

    connect(m_pausePlayAction, &QAction::triggered, this, &SquishPerspective::onPausePlayTriggered);
    connect(m_stepInAction, &QAction::triggered, this, [this] {
        emit runRequested(StepMode::StepIn);
    });
    connect(m_stepOverAction, &QAction::triggered, this, [this] {
        emit runRequested(StepMode::StepOver);
    });
    connect(m_stepOutAction, &QAction::triggered, this, [this] {
        emit runRequested(StepMode::StepOut);
    });
    connect(m_stopAction, &QAction::triggered, this, &SquishPerspective::onStopTriggered);
    connect(m_stopRecordAction, &QAction::triggered,
            this, &SquishPerspective::onStopRecordTriggered);

    connect(SquishTools::instance(), &SquishTools::localsUpdated,
            this, &SquishPerspective::onLocalsUpdated);
    connect(localsView, &QTreeView::expanded, this, [this](const QModelIndex &idx) {
        LocalsItem *item = m_localsModel.itemForIndex(idx);
        if (QTC_GUARD(item)) {
            if (item->expanded)
                return;
            item->expanded = true;
            SquishTools::instance()->requestExpansion(item->name);
        }
    });
}

void SquishPerspective::onStopTriggered()
{
    m_stopRecordAction->setEnabled(false);
    m_pausePlayAction->setEnabled(false);
    m_stopAction->setEnabled(false);
    emit stopRequested();
}

void SquishPerspective::onStopRecordTriggered()
{
    m_stopRecordAction->setEnabled(false);
    m_pausePlayAction->setEnabled(false);
    m_stopAction->setEnabled(false);
    emit stopRecordRequested();
}

void SquishPerspective::onPausePlayTriggered()
{
    if (m_mode == Interrupted)
        emit runRequested(StepMode::Continue);
    else if (m_mode == Running)
        emit interruptRequested();
    else
        qDebug() << "###state: " << m_mode;
}

void SquishPerspective::onLocalsUpdated(const QString &output)
{
    static const QRegularExpression regex("\\+(?<name>.+):\\{(?<content>.*)\\}");
    static const QRegularExpression inner("(?<type>.+)#(?<exp>\\+*+)(?<name>[^=]+)(=(?<value>.+))?");
    const QRegularExpressionMatch match = regex.match(output);
    LocalsItem *parent = nullptr;

    bool singleSymbol = match.hasMatch();
    if (singleSymbol) {
        const QString name = match.captured("name");
        parent = m_localsModel.findNonRootItem([name](LocalsItem *it) {
                return it->name == name;
        });
        if (!parent)
            return;
        parent->removeChildren();
    } else {
        m_localsModel.clear();
        parent = m_localsModel.rootItem();
    }
    const QStringList symbols = splitDebugContent(singleSymbol ? match.captured("content")
                                                               : output);
    for (const QString &part : symbols) {
        const QRegularExpressionMatch iMatch = inner.match(part);
        QTC_ASSERT(iMatch.hasMatch(), qDebug() << part; continue);
        if (iMatch.captured("value").startsWith('<'))
            continue;
        LocalsItem *l = new LocalsItem(iMatch.captured("name"),
                                       iMatch.captured("type"),
                                       iMatch.captured("value").replace("\\,", ",")); // TODO
        if (!iMatch.captured("exp").isEmpty())
            l->appendChild(new LocalsItem); // add pseudo child
        parent->appendChild(l);
    }
}

void SquishPerspective::updateStatus(const QString &status)
{
    m_status->setText(status);
    if (m_controlBar)
        m_controlBar->updateProgressText(status);
}

void SquishPerspective::showControlBar(SquishXmlOutputHandler *xmlOutputHandler)
{
    QTC_ASSERT(!m_controlBar, return);
    m_controlBar = new SquishControlBar(this);

    if (xmlOutputHandler) {
        connect(xmlOutputHandler, &SquishXmlOutputHandler::increasePassCounter,
                m_controlBar, &SquishControlBar::increasePassCounter);
        connect(xmlOutputHandler, &SquishXmlOutputHandler::increaseFailCounter,
                m_controlBar, &SquishControlBar::increaseFailCounter);
        connect (xmlOutputHandler, &SquishXmlOutputHandler::updateStatus,
                 m_controlBar, &SquishControlBar::updateProgressText);
    }

    const QRect rect = Core::ICore::dialogParent()->screen()->availableGeometry();
    m_controlBar->move(rect.width() - m_controlBar->width() - 10, 10);
    m_controlBar->showNormal();
}

void SquishPerspective::destroyControlBar()
{
    if (!m_controlBar)
        return;
    delete m_controlBar;
    m_controlBar = nullptr;
}

void SquishPerspective::setPerspectiveMode(PerspectiveMode mode)
{
    if (m_mode == mode) // ignore
        return;

    m_mode = mode;
    switch (m_mode) {
    case Running:
        m_stopRecordAction->setEnabled(false);
        m_pausePlayAction->setEnabled(true);
        m_pausePlayAction->setIcon(iconForType(IconType::Pause));
        m_pausePlayAction->setToolTip(Tr::tr("Interrupt"));
        m_stepInAction->setEnabled(false);
        m_stepOverAction->setEnabled(false);
        m_stepOutAction->setEnabled(false);
        m_stopAction->setEnabled(true);
        break;
    case Recording:
        m_stopRecordAction->setEnabled(true);
        m_pausePlayAction->setEnabled(false);
        m_pausePlayAction->setIcon(iconForType(IconType::Pause));
        m_pausePlayAction->setToolTip(Tr::tr("Interrupt"));
        m_stepInAction->setEnabled(false);
        m_stepOverAction->setEnabled(false);
        m_stepOutAction->setEnabled(false);
        m_stopAction->setEnabled(true);
        break;
    case Interrupted:
        m_pausePlayAction->setEnabled(true);
        m_pausePlayAction->setIcon(iconForType(IconType::Play));
        m_pausePlayAction->setToolTip(Tr::tr("Continue"));
        m_stepInAction->setEnabled(true);
        m_stepOverAction->setEnabled(true);
        m_stepOutAction->setEnabled(true);
        m_stopAction->setEnabled(true);
        break;
    case Querying:
    case NoMode:
        m_pausePlayAction->setIcon(iconForType(IconType::Pause));
        m_pausePlayAction->setToolTip(Tr::tr("Interrupt"));
        m_pausePlayAction->setEnabled(false);
        m_stopRecordAction->setEnabled(false);
        m_stepInAction->setEnabled(false);
        m_stepOverAction->setEnabled(false);
        m_stepOutAction->setEnabled(false);
        m_stopAction->setEnabled(false);
        m_localsModel.clear();
        break;
    default:
        break;
    }
}

} // namespace Internal
} // namespace Squish
