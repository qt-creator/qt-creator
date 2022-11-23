// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishprocessbase.h"

namespace Squish::Internal {

SquishProcessBase::SquishProcessBase(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &Utils::QtcProcess::readyReadStandardError,
            this, &SquishProcessBase::onErrorOutput);
    connect(&m_process, &Utils::QtcProcess::done,
            this, &SquishProcessBase::onDone);
}

void SquishProcessBase::setState(SquishProcessState state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

} // namespace Squish::Internal
