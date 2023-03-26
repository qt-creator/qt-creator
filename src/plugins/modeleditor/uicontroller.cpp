// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uicontroller.h"

#include "modeleditor_constants.h"

#include <QSettings>

namespace ModelEditor {
namespace Internal {

class UiController::UiControllerPrivate
{
public:
    QByteArray rightSplitterState;
    QByteArray rightHorizSplitterState;
};

UiController::UiController()
    : d(new UiControllerPrivate)
{
}

UiController::~UiController()
{
    delete d;
}

bool UiController::hasRightSplitterState() const
{
    return !d->rightSplitterState.isEmpty();
}

QByteArray UiController::rightSplitterState() const
{
    return d->rightSplitterState;
}

bool UiController::hasRightHorizSplitterState() const
{
    return  !d->rightHorizSplitterState.isEmpty();
}

QByteArray UiController::rightHorizSplitterState() const
{
    return d->rightHorizSplitterState;
}

void UiController::onRightSplitterChanged(const QByteArray &state)
{
    d->rightSplitterState = state;
    emit rightSplitterChanged(state);
}

void UiController::onRightHorizSplitterChanged(const QByteArray &state)
{
    d->rightHorizSplitterState = state;
    emit rightHorizSplitterChanged(state);
}

void UiController::saveSettings(QSettings *settings)
{
    if (hasRightSplitterState()) {
        settings->setValue(
                    QLatin1String(Constants::SETTINGS_RIGHT_SPLITTER), d->rightSplitterState);
    }
    if (hasRightHorizSplitterState()) {
        settings->setValue(
                    QLatin1String(Constants::SETTINGS_RIGHT_HORIZ_SPLITTER),
                    d->rightHorizSplitterState);
    }
}

void UiController::loadSettings(QSettings *settings)
{
    if (settings->contains(QLatin1String(Constants::SETTINGS_RIGHT_SPLITTER))) {
        d->rightSplitterState = settings->value(
                    QLatin1String(Constants::SETTINGS_RIGHT_SPLITTER)).toByteArray();
    }
    if (settings->contains(QLatin1String(Constants::SETTINGS_RIGHT_HORIZ_SPLITTER))) {
        d->rightHorizSplitterState = settings->value(
                    QLatin1String(Constants::SETTINGS_RIGHT_HORIZ_SPLITTER)).toByteArray();
    }
}

} // namespace Internal
} // namespace ModelEditor
