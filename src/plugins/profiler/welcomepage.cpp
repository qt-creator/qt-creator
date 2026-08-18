// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "welcomepage.h"

#include "profilertr.h"

#include <utils/layoutbuilder.h>
#include <utils/qtdesignwidgets.h>

#include <QVBoxLayout>

using namespace Layouting;
using namespace Utils;
using namespace Utils::StyleHelper;

namespace Profiler::Internal {

WelcomePage::WelcomePage(QWidget *parent)
    : QWidget(parent)
{
    m_backendCombo = new QtcComboBox;
    m_backendCombo->setObjectName("ProfilerBackendCombo");
    connect(m_backendCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        emit backendChanged(index);
    });

    m_targetCombo = new QtcComboBox;
    m_targetCombo->setObjectName("ProfilerTargetCombo");
    connect(m_targetCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        emit targetChanged(index);
    });
    m_targetRow = Row { Tr::tr("Profile:"), m_targetCombo, st }.emerge();
    m_targetRow->hide(); // Until a frontend offers something to choose between.

    auto configHost = new QWidget;
    m_configLayout = new QVBoxLayout(configHost);
    m_configLayout->setContentsMargins(0, 0, 0, 0);

    m_startButton = new QtcButton(Tr::tr("Start Recording"), QtcButton::LargePrimary);
    m_startButton->setToolTip(Tr::tr("Start recording with the selected backend."));
    connect(m_startButton, &QAbstractButton::clicked,
            this, [this] { emit startRecordingRequested(); });

    // clang-format off
    Row {
        st,
        Column {
            st,
            Row { Tr::tr("Backend:"), m_backendCombo, st },
            Space(SpacingTokens::PrimitiveM),
            m_targetRow,
            Space(SpacingTokens::PrimitiveM),
            // The active backend's own controls, including how it starts.
            configHost,
            Space(SpacingTokens::PrimitiveM),
            m_startButton,
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

void WelcomePage::setTargets(const QStringList &names, int current)
{
    QSignalBlocker blocker(m_targetCombo);
    m_targetCombo->clear();
    m_targetCombo->addItems(names);
    if (current >= 0 && current < names.size())
        m_targetCombo->setCurrentIndex(current);
    m_targetRow->setVisible(!names.isEmpty());
}

void WelcomePage::setCurrentBackend(int index)
{
    m_backendCombo->setCurrentIndex(index); // emits currentIndexChanged -> backendChanged
}

void WelcomePage::setStartEnabled(bool enabled, const QString &toolTip)
{
    m_startButton->setEnabled(enabled);
    m_startButton->setToolTip(toolTip.isEmpty()
                                  ? Tr::tr("Start recording with the selected backend.")
                                  : toolTip);
}

void WelcomePage::setActiveBackend(QWidget *configWidget)
{
    delete m_configWidget;
    m_configWidget = configWidget;
    if (m_configWidget)
        m_configLayout->addWidget(m_configWidget);
}

} // namespace Profiler::Internal
