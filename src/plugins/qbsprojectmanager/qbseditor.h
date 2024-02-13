// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljseditor/qmljseditor.h>

namespace QbsProjectManager::Internal {

class QbsEditorFactory : public QmlJSEditor::QmlJSEditorFactory
{
public:
    QbsEditorFactory();
};

} // namespace QbsProjectManager::Internal
