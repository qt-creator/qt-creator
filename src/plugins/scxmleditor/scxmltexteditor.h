// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "scxmleditor_global.h"
#include <texteditor/texteditor.h>

namespace ScxmlEditor {
namespace Internal { class ScxmlEditorDocument; }

class ScxmlTextEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    ScxmlTextEditor();

    void finalizeInitialization() override;
    bool open(QString *errorString, const QString &fileName, const QString &realFileName);

    QWidget *toolBar() override
    {
        return nullptr;
    }

    bool isDesignModePreferred() const override
    {
        return true;
    }
};

} // namespace ScxmlEditor
