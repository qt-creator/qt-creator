// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../beautifiertool.h"

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Beautifier::Internal {

class ArtisticStyle : public BeautifierTool
{
public:
    ArtisticStyle();

    QString id() const override;
    void updateActions(Core::IEditor *editor) override;
    TextEditor::Command command() const override;
    bool isApplicable(const Core::IDocument *document) const override;

private:
    void formatFile();
    QString configurationFile() const;
    TextEditor::Command command(const QString &cfgFile) const;

    QAction *m_formatFile = nullptr;
};

} // Beautifier::Internal
