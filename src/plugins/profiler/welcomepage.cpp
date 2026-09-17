// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "welcomepage.h"

#include "profilertr.h"

#include <utils/layoutbuilder.h>
#include <utils/qtdesignwidgets.h>

#include <QScrollArea>
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
    m_targetRow = Row {
        Tr::tr("Profile:"),
        m_targetCombo,
        spacing(SpacingTokens::GapHM),
        noMargin,
    }.emerge();
    m_targetRow->hide(); // Until a frontend offers something to choose between.

    const int bigSpacing = SpacingTokens::PrimitiveXxl;
    const int rectRadius = SpacingTokens::RadiusL;
    const int innerMargin = qMax(0, rectRadius - SpacingTokens::PrimitiveM);

    auto configHost = new QWidget;
    configHost->setContentsMargins({});
    StyleHelper::setBackgroundColor(configHost, Theme::Token_Background_Muted);
    m_configLayout = new QVBoxLayout(configHost);
    m_configLayout->setContentsMargins(innerMargin, innerMargin, innerMargin, innerMargin);
    m_configLayout->addStretch();

    auto configScrollArea = new QScrollArea;
    configScrollArea->setFrameShape(QFrame::NoFrame);
    configScrollArea->setFixedWidth(740);
    configScrollArea->setWidget(configHost);
    configScrollArea->setWidgetResizable(true);

    m_startButton = new QtcButton(Tr::tr("Start Recording"), QtcButton::LargePrimary);
    m_startButton->setToolTip(Tr::tr("Start recording with the selected backend."));
    connect(m_startButton, &QAbstractButton::clicked,
            this, [this] { emit startRecordingRequested(); });

    // clang-format off
    Row {
        customMargins(bigSpacing, bigSpacing, bigSpacing, bigSpacing),
        st,
        Column {
            Row {
                Row {
                    Tr::tr("Backend:"),
                    m_backendCombo,
                    spacing(SpacingTokens::GapHM),
                },
                m_targetRow,
                st,
                spacing(bigSpacing),
            },
            QtDesignWidgets::Rectangle {
                fillBrush(creatorColor(Theme::Token_Background_Muted)),
                strokePen(creatorColor(Theme::Token_Stroke_Subtle)),
                radius(rectRadius),
                Row {
                    configScrollArea,
                    noMargin,
                },
            },
            Row {
                st,
                m_startButton,
            },
            spacing(bigSpacing),
            noMargin,
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
        m_configLayout->insertWidget(0, m_configWidget);
}

} // namespace Profiler::Internal
