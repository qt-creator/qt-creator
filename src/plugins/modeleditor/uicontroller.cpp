/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
                    QLatin1String(Constants::SETTINGS_RIGHT_SPLITTER)).value<QByteArray>();
    }
    if (settings->contains(QLatin1String(Constants::SETTINGS_RIGHT_HORIZ_SPLITTER))) {
        d->rightHorizSplitterState = settings->value(
                    QLatin1String(Constants::SETTINGS_RIGHT_HORIZ_SPLITTER)).value<QByteArray>();
    }
}

} // namespace Internal
} // namespace ModelEditor
