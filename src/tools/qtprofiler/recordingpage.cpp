// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "recordingpage.h"

#include <profiler/profilertr.h>

#include <utils/layoutbuilder.h>
#include <utils/qtdesignwidgets.h>

#include <QProgressBar>
#include <QTime>
#include <QTimer>

using namespace Layouting;
using namespace Profiler;
using namespace Utils;
using namespace Utils::StyleHelper;
using namespace Qt::StringLiterals;

namespace QtProfiler {

RecordingPage::RecordingPage(QWidget *parent)
    : QWidget(parent)
{
    m_tick = new QTimer(this);
    m_tick->setInterval(100);
    connect(m_tick, &QTimer::timeout, this, &RecordingPage::updateElapsed);

    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setFixedWidth(240);
    m_progressBar->hide(); // only shown while processing

    // clang-format off
    Row {
        st,
        Column {
            st,
            QtDesignWidgets::Label {
                bindTo(&m_titleLabel),
                role(QtcLabel::Primary),
                text(Tr::tr("Recording...")),
            },
            Space(SpacingTokens::PrimitiveM),
            QtDesignWidgets::Label {
                bindTo(&m_timerLabel),
                role(QtcLabel::Secondary),
                text("00:00.0"),
            },
            Space(SpacingTokens::PrimitiveL),
            m_progressBar,
            Space(SpacingTokens::PrimitiveL),
            QtDesignWidgets::Button {
                bindTo(&m_stopButton),
                role(QtcButton::LargePrimary),
                text(Tr::tr("Stop Recording")),
                Layouting::toolTip(Tr::tr("Stop recording and load the captured trace.")),
                onClicked(this, [this] { emit stopRequested(); }),
            },
            st,
        }
        ,
        st,
    }.attachTo(this);
    // clang-format on
}

void RecordingPage::start(const QString &processName)
{
    m_titleLabel->setText(Tr::tr("Recording %1...").arg(processName));
    m_stopButton->setEnabled(true);
    m_stopButton->setText(Tr::tr("Stop Recording"));
    m_progressBar->hide();
    m_progressBar->setValue(0);
    m_elapsed.restart();
    updateElapsed();
    m_tick->start();
}

void RecordingPage::setProcessing()
{
    // The worker is still symbolizing samples and writing the trace; freeze the
    // timer and disable the button so the click registers immediately.
    m_tick->stop();
    m_titleLabel->setText(Tr::tr("Processing captured samples..."));
    m_stopButton->setEnabled(false);
    m_stopButton->setText(Tr::tr("Stopping..."));
    m_progressBar->setValue(0);
    m_progressBar->show();
}

void RecordingPage::setProgress(int percent)
{
    m_progressBar->setValue(percent);
}

void RecordingPage::stop()
{
    m_tick->stop();
}

void RecordingPage::updateElapsed()
{
    const qint64 ms = m_elapsed.elapsed();
    const QTime t = QTime::fromMSecsSinceStartOfDay(int(ms % (24 * 60 * 60 * 1000)));
    const QString format = ms >= 60 * 60 * 1000 ? "HH:mm:ss"_L1 : "mm:ss.z"_L1;
    m_timerLabel->setText(t.toString(format));
}

} // namespace QtProfiler
