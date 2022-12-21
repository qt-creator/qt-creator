// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QPlainTextEdit>

#include "plaintexteditmodifier.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT ByteArrayModifier: public PlainTextEditModifier
{
public:
    static ByteArrayModifier* create(const QString& data);

    ByteArrayModifier(QPlainTextEdit* textEdit);
    ~ByteArrayModifier();

    void setText(const QString& text);

    void undo();
    void redo();

private:
    QPlainTextEdit* m_textEdit;
};

}
