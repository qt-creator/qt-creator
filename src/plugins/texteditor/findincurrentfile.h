// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "basefilefind.h"

#include <QPointer>

namespace Core {
class IEditor;
class IDocument;
} // namespace Core

namespace TextEditor {
namespace Internal {

class FindInCurrentFile final : public BaseFileFind
{
    Q_OBJECT

public:
    FindInCurrentFile();

    QString id() const override;
    QString displayName() const override;
    bool isEnabled() const override;
    void writeSettings(Utils::QtcSettings *settings) override;
    void readSettings(Utils::QtcSettings *settings) override;

protected:
    QString label() const override;
    QString toolTip() const override;

private:
    FileContainerProvider fileContainerProvider() const override;
    void handleFileChange(Core::IEditor *editor);

    QPointer<Core::IDocument> m_currentDocument;
};

} // namespace Internal
} // namespace TextEditor
