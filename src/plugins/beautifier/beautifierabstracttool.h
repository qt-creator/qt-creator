// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/command.h>

#include <QObject>

namespace Core {
class IDocument;
class IEditor;
}

namespace Beautifier {
namespace Internal {

class BeautifierAbstractTool : public QObject
{
    Q_OBJECT

public:
    BeautifierAbstractTool() = default;

    virtual QString id() const = 0;
    virtual void updateActions(Core::IEditor *editor) = 0;

    /**
     * Returns the tool's command to format an entire file.
     *
     * @note    The received command may be invalid.
     */
    virtual TextEditor::Command command() const = 0;

    virtual bool isApplicable(const Core::IDocument *document) const = 0;
};

} // namespace Internal
} // namespace Beautifier
