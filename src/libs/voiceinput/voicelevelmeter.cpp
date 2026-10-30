// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "voicelevelmeter.h"

#include "speechactivity.h"
#include "transcriber.h"

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QPainter>

using namespace Utils;
using namespace Utils::StyleHelper::SpacingTokens;

namespace VoiceInput {

const int kBars = 20;
// Fewer still show that the recording is heard, where a row has no more room.
const int kMinimumBars = 6;
const int kBarWidth = GapHXxs;
const int kBarGap = GapHXs;

class VoiceLevelMeterPrivate
{
public:
    Internal::LevelHistory history{kBars};
};

/*!
    \class VoiceInput::VoiceLevelMeter
    \inmodule QtCreator

    \brief The VoiceLevelMeter class shows how loud the recording of a
    Transcriber is.

    It is visible only while the transcriber records. The newest level is on
    the right.
*/

VoiceLevelMeter::VoiceLevelMeter(Transcriber *transcriber, QWidget *parent)
    : QWidget(parent)
    , d(new VoiceLevelMeterPrivate)
{
    setVisible(transcriber->state() != Transcriber::State::Idle);
    connect(transcriber, &Transcriber::stateChanged, this, [this](Transcriber::State state) {
        if (state == Transcriber::State::Recording)
            d->history.clear();
        setVisible(state != Transcriber::State::Idle);
        update();
    });
    connect(transcriber, &Transcriber::levelMeasured, this,
            [this](qint64 firstSample, float level) {
        d->history.addLevel(firstSample, level);
        update();
    });
}

VoiceLevelMeter::~VoiceLevelMeter()
{
    delete d;
}

QSize VoiceLevelMeter::sizeHint() const
{
    return {kBars * (kBarWidth + kBarGap), fontMetrics().height() * 3 / 2};
}

QSize VoiceLevelMeter::minimumSizeHint() const
{
    return {kMinimumBars * (kBarWidth + kBarGap), sizeHint().height()};
}

void VoiceLevelMeter::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(creatorColor(Theme::Token_Text_Muted));
    const qreal middle = height() / 2.0;
    const qreal maxHeight = height();
    qreal x = width() - kBarWidth;
    const QList<Internal::LevelHistory::Bar> &bars = d->history.bars();
    for (auto bar = bars.crbegin(); bar != bars.crend() && x >= 0; ++bar) {
        // A bar for silence is a dot.
        const qreal barHeight = std::max<qreal>(kBarWidth,
                                                Internal::levelHeight(bar->level) * maxHeight);
        painter.drawRoundedRect(QRectF(x, middle - barHeight / 2, kBarWidth, barHeight),
                                kBarWidth / 2.0, kBarWidth / 2.0);
        x -= kBarWidth + kBarGap;
    }
}

} // namespace VoiceInput
