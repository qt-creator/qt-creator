// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "codeassist/iassistprovider.h"


namespace TextEditor {
namespace Internal {

class ClipboardAssistProvider: public IAssistProvider
{
public:
    ClipboardAssistProvider(QObject *parent = nullptr) : IAssistProvider(parent) {}
    IAssistProcessor *createProcessor(const AssistInterface *) const override;
};

} // namespace Internal
} // namespace TextEditor
