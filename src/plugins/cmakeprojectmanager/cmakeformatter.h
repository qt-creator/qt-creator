// Copyright (C) 2016 Lorenz Haas
// Copyright (C) 2022 Xavier BESSON
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/command.h>

#include "cmakeformatteroptionspage.h"

namespace Core {
class IDocument;
class IEditor;
}

namespace CMakeProjectManager {
namespace Internal {

class CMakeFormatter : public QObject
{
    Q_OBJECT

public:
    void updateActions(Core::IEditor *editor);
    TextEditor::Command command() const;
    bool isApplicable(const Core::IDocument *document) const;

    void initialize();

private:
    void formatFile();

    QAction *m_formatFile = nullptr;
    CMakeFormatterOptionsPage m_page;
};

} // namespace Internal
} // namespace CMakeProjectManager
