/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

UiController::UiController(QObject *parent)
    : QObject(parent),
      d(new UiControllerPrivate)
{
}

UiController::~UiController()
{
    delete d;
}

bool UiController::hasRightSplitterState() const
{
    return d->rightSplitterState.size() > 0;
}

QByteArray UiController::rightSplitterState() const
{
    return d->rightSplitterState;
}

bool UiController::hasRightHorizSplitterState() const
{
    return  d->rightHorizSplitterState.size() > 0;
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
