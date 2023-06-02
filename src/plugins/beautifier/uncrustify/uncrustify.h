// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../beautifierabstracttool.h"

#include "uncrustifysettings.h"

namespace Beautifier::Internal {

class Uncrustify : public BeautifierAbstractTool
{
public:
    Uncrustify();

    QString id() const override;
    void updateActions(Core::IEditor *editor) override;
    TextEditor::Command command() const override;
    bool isApplicable(const Core::IDocument *document) const override;

private:
    void formatFile();
    void formatSelectedText();
    QString configurationFile() const;
    TextEditor::Command command(const QString &cfgFile, bool fragment = false) const;

    QAction *m_formatFile = nullptr;
    QAction *m_formatRange = nullptr;
    UncrustifySettings m_settings;
    UncrustifyOptionsPage m_page{&m_settings};
};

} // Beautifier::Internal
