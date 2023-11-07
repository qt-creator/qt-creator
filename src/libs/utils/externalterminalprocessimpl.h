// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "terminalinterface.h"

namespace Utils {

class ProcessStubCreator;

class QTCREATOR_UTILS_EXPORT ExternalTerminalProcessImpl final : public TerminalInterface
{
public:
    ExternalTerminalProcessImpl();

    static QString openTerminalScriptAttached();
};

class QTCREATOR_UTILS_EXPORT ProcessStubCreator : public StubCreator
{
public:
    ProcessStubCreator(TerminalInterface *interface);
    ~ProcessStubCreator() override = default;

    expected_str<qint64> startStubProcess(const ProcessSetupData &setupData) override;

    TerminalInterface *m_interface;
};

} // namespace Utils
