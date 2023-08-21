// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditorfactory.h>

#include <QObject>

namespace QtSupport::Internal {

class DesignerExternalEditor : public Core::IEditorFactory
{
public:
    DesignerExternalEditor();

private:
    QObject m_guard;
};

class LinguistEditor : public Core::IEditorFactory
{
public:
    LinguistEditor();
};

} // QtSupport::Internal
