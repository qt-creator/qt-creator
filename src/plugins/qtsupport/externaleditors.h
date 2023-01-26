// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace QtSupport::Internal {

class DesignerEditorFactory : public QObject
{
public:
    DesignerEditorFactory();
};

class LinguistEditorFactory : public QObject
{
public:
    LinguistEditorFactory();
};

} // QtSupport::Internal
