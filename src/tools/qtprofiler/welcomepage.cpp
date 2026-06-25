// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "welcomepage.h"

#include <profiler/profilertr.h>

#include <utils/layoutbuilder.h>
#include <utils/qtdesignwidgets.h>

#include <QVBoxLayout>

using namespace Layouting;
using namespace Profiler;
using namespace Utils;
using namespace Utils::StyleHelper;

namespace QtProfiler {

WelcomePage::WelcomePage(QWidget *parent)
    : QWidget(parent)
{
    m_backendCombo = new QtcComboBox;
    connect(m_backendCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        emit backendChanged(index);
    });

    auto configHost = new QWidget;
    m_configLayout = new QVBoxLayout(configHost);
    m_configLayout->setContentsMargins(0, 0, 0, 0);

    auto startButton = new QtcButton(Tr::tr("Start Recording"), QtcButton::LargePrimary);
    startButton->setToolTip(Tr::tr("Start recording with the selected backend."));
    connect(startButton, &QAbstractButton::clicked, this, [this] { emit startRecordingRequested(); });

    // clang-format off
    Row {
        st,
        Column {
            st,
            Row { Tr::tr("Backend:"), m_backendCombo, st },
            Space(SpacingTokens::PrimitiveM),
            // The active backend's own controls, including how it starts.
            configHost,
            Space(SpacingTokens::PrimitiveM),
            startButton,
            st,
        },
        st,
    }.attachTo(this);
    // clang-format on
}

void WelcomePage::setBackends(const QStringList &names, int current)
{
    QSignalBlocker blocker(m_backendCombo);
    m_backendCombo->clear();
    m_backendCombo->addItems(names);
    if (current >= 0 && current < names.size())
        m_backendCombo->setCurrentIndex(current);
}

void WelcomePage::setCurrentBackend(int index)
{
    m_backendCombo->setCurrentIndex(index); // emits currentIndexChanged -> backendChanged
}

void WelcomePage::setActiveBackend(QWidget *configWidget)
{
    delete m_configWidget;
    m_configWidget = configWidget;
    if (m_configWidget)
        m_configLayout->addWidget(m_configWidget);
}

} // namespace QtProfiler
