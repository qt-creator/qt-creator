// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>

#include <qmljseditor/qmljsfindreferences.h>

class FindImplementation
{
public:
    FindImplementation();
    static QList<QmlJSEditor::FindReferences::Usage> run(const QString &fileName, const QString &typeName, const QString &itemName);
};
