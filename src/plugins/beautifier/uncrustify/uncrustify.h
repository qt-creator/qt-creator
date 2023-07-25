// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../beautifiertool.h"

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Beautifier::Internal {

class Uncrustify : public BeautifierTool
{
public:
    Uncrustify();

    QString id() const override;
    void updateActions(Core::IEditor *editor) override;
    TextEditor::Command textCommand() const override;
    bool isApplicable(const Core::IDocument *document) const override;

private:
    void formatFile();
    void formatSelectedText();
    Utils::FilePath configurationFile() const;
    TextEditor::Command textCommand(const Utils::FilePath &cfgFile, bool fragment = false) const;

    QAction *m_formatFile = nullptr;
    QAction *m_formatRange = nullptr;
};

} // Beautifier::Internal
