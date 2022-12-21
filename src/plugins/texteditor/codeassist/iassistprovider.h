// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor_global.h>

#include <QObject>

namespace TextEditor {

class AssistInterface;
class IAssistProcessor;

class TEXTEDITOR_EXPORT IAssistProvider : public QObject
{
    Q_OBJECT

public:
    IAssistProvider(QObject *parent = nullptr) : QObject(parent) {}
    virtual IAssistProcessor *createProcessor(const AssistInterface *assistInterface) const = 0;
};

} // TextEditor
