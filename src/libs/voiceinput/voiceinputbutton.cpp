// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "voiceinputbutton.h"

#include "transcriber.h"
#include "voiceinputtr.h"

#include <utils/icon.h>
#include <utils/plaintextedit/plaintextedit.h>
#include <utils/theme/theme.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>

#include <algorithm>
#include <utility>

using namespace Utils;

namespace VoiceInput {

// A smaller icon inside a ring segment rotated by angle degrees.
static QIcon loadingIcon(const QIcon &icon, int angle)
{
    const QSize size(16, 16);
    QIcon result;
    for (const qreal dpr : {1.0, 2.0}) {
        QPixmap pixmap(size * dpr);
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawPixmap(QRect(3, 3, 10, 10), icon.pixmap(QSize(10, 10), dpr));
        QPen pen(creatorColor(Theme::Token_Text_Accent), 1.5);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawArc(QRectF(1, 1, 14, 14), -angle * 16, 100 * 16);
        result.addPixmap(pixmap);
    }
    return result;
}

class VoiceInputButtonPrivate
{
public:
    explicit VoiceInputButtonPrivate(VoiceInputButton *q)
        : q(q)
        , transcriber(new Transcriber(q))
    {}

    void showSettingsMenu()
    {
        // A download started from the menu may outlive the button.
        Transcriber::showSettingsMenu(q, q->mapToGlobal(QPoint(0, q->height())),
                                      [button = QPointer<VoiceInputButton>(q)] {
            if (button)
                button->d->update();
        });
    }

    void startTranscript()
    {
        if (!textEdit)
            return;
        QTextCursor cursor = textEdit->textCursor();
        cursor.clearSelection();
        transcriptStart = cursor.position();
        transcriptLength = 0;
        const QChar before = transcriptStart > 0
                                 ? textEdit->document()->characterAt(transcriptStart - 1)
                                 : QChar();
        transcriptPrefix = before.isNull() || before.isSpace() ? QString() : QString(" ");
    }

    void insertTranscript(const QString &text)
    {
        if (!textEdit)
            return;
        QTextDocument *document = textEdit->document();
        const int end = document->characterCount() - 1;
        const int start = std::min(transcriptStart, end);
        QTextCursor cursor(document);
        cursor.setPosition(start);
        cursor.setPosition(std::min(start + transcriptLength, end), QTextCursor::KeepAnchor);
        const QString replacement = text.isEmpty() ? QString() : transcriptPrefix + text;
        cursor.insertText(replacement);
        transcriptStart = start;
        transcriptLength = replacement.size();
        textEdit->setTextCursor(cursor);
    }

    void update()
    {
        const Transcriber::State state = transcriber->state();
        const bool active = state != Transcriber::State::Idle;
        const bool loading = transcriber->isLoadingModel();
        const Icon icon({{":/voiceinput/images/microphone.png",
                          active ? Theme::IconsStopColor : Theme::PaletteText}},
                        Icon::Tint);
        if (loading) {
            if (!loadingTimer.isActive())
                loadingTimer.start();
            q->setIcon(loadingIcon(icon.icon(), loadingAngle));
        } else {
            loadingTimer.stop();
            loadingAngle = 0;
            q->setIcon(icon.icon());
        }
        q->setEnabled(state != Transcriber::State::Finishing);
        QString toolTip;
        if (loading) {
            toolTip = Tr::tr("Loading the speech recognition model...\n"
                             "What you say meanwhile is transcribed once it is loaded.");
        } else if (state == Transcriber::State::Recording) {
            toolTip = holding ? Tr::tr("Release to Stop Voice Input\n"
                                       "The transcribed text stays for you to edit.")
                              : Tr::tr("Stop Voice Input\n"
                                       "The transcribed text stays for you to edit.");
        } else if (state == Transcriber::State::Finishing) {
            toolTip = Tr::tr("Transcribing the end of the recording...");
        } else if (Transcriber::hasModel() && Transcriber::holdsToRecord()) {
            toolTip = Tr::tr("Hold for Voice Input (%1)\n"
                             "Transcribes what you say into the text while you hold the button.\n"
                             "Right-click for the model and other settings.")
                          .arg(Transcriber::modelName());
        } else if (Transcriber::hasModel()) {
            toolTip = Tr::tr("Start Voice Input (%1)\n"
                             "Transcribes what you say into the text.\n"
                             "Right-click for the model and other settings.")
                          .arg(Transcriber::modelName());
        } else {
            toolTip = Tr::tr("Set Up Voice Input\n"
                             "Downloads a speech recognition model to transcribe what you say "
                             "into the text.");
        }
        q->setToolTip(toolTip);
    }

    VoiceInputButton *q;
    Transcriber *transcriber;
    QPointer<PlainTextEdit> textEdit;
    // The part of the text the running transcript replaces as it grows.
    int transcriptStart = 0;
    int transcriptLength = 0;
    QString transcriptPrefix;
    // Whether the button is held to record, and whether that started the recording.
    bool holding = false;
    bool startedByHolding = false;
    // Loading a model compiles GPU kernels first, which takes seconds and
    // reports no progress.
    QTimer loadingTimer;
    int loadingAngle = 0;
};

/*!
    \class VoiceInput::VoiceInputButton
    \inmodule QtCreator

    \brief The VoiceInputButton class is a microphone button that transcribes
    what the user says into a text edit.

    Holding the button down records, or, without Hold to Record, a click starts
    recording and the next one stops it. The first click, and a right click,
    show a menu for selecting and downloading a speech recognition model, and
    for the other voice input settings.
*/

VoiceInputButton::VoiceInputButton(QWidget *parent)
    : QtcIconButton(parent)
    , d(new VoiceInputButtonPrivate(this))
{
    connect(this, &QAbstractButton::clicked, this, [this] {
        // A press started the recording, and the release stops it.
        if (d->holding)
            return;
        if (d->transcriber->state() != Transcriber::State::Idle)
            d->transcriber->stop();
        else if (Transcriber::hasModel())
            d->transcriber->start(this);
        else
            d->showSettingsMenu();
        if (d->textEdit)
            d->textEdit->setFocus();
    });
    d->loadingTimer.setInterval(50);
    connect(&d->loadingTimer, &QTimer::timeout, this, [this] {
        d->loadingAngle = (d->loadingAngle + 20) % 360;
        d->update();
    });

    connect(d->transcriber, &Transcriber::stateChanged, this, [this](Transcriber::State state) {
        if (state == Transcriber::State::Recording) {
            d->startTranscript();
            // Asking for the microphone may take longer than the user held the button.
            if (d->startedByHolding && !d->holding)
                d->transcriber->stop();
        } else if (state == Transcriber::State::Idle) {
            d->startedByHolding = false;
        }
        d->update();
    });
    connect(d->transcriber, &Transcriber::loadingModelChanged, this, [this] { d->update(); });
    connect(d->transcriber, &Transcriber::transcriptChanged, this, [this](const QString &text) {
        d->insertTranscript(text);
    });
    connect(d->transcriber, &Transcriber::errorOccurred, this, &VoiceInputButton::errorOccurred);
    d->update();
}

VoiceInputButton::~VoiceInputButton()
{
    // Its signals reach into d.
    delete d->transcriber;
    delete d;
}

// On the press itself, so that it does not depend on how the platform turns a
// right click into a context menu event.
void VoiceInputButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        event->accept();
        d->showSettingsMenu();
        return;
    }
    QtcIconButton::mousePressEvent(event);
    if (event->button() == Qt::LeftButton && Transcriber::holdsToRecord()
        && Transcriber::hasModel() && d->transcriber->state() == Transcriber::State::Idle) {
        d->holding = true;
        d->startedByHolding = true;
        d->transcriber->start(this);
        if (d->textEdit)
            d->textEdit->setFocus();
        d->update();
    }
}

void VoiceInputButton::mouseReleaseEvent(QMouseEvent *event)
{
    // The clicked() this may emit sees that the button was held.
    QtcIconButton::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton && std::exchange(d->holding, false)) {
        d->transcriber->stop();
        d->update();
    }
}

/*!
    Inserts what the user says into \a textEdit at its cursor.
*/
void VoiceInputButton::setTextEdit(PlainTextEdit *textEdit)
{
    d->textEdit = textEdit;
}

/*!
    Returns the transcriber, for following the recording or the transcript.
*/
Transcriber *VoiceInputButton::transcriber() const
{
    return d->transcriber;
}

} // namespace VoiceInput
