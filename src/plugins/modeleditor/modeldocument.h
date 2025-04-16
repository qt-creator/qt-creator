// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/idocument.h>
#include <utils/filepath.h>

namespace qmt { class Uid; }

namespace ModelEditor::Internal {

class ExtDocumentController;

class ModelDocument : public Core::IDocument
{
    Q_OBJECT
    class ModelDocumentPrivate;

public:
    explicit ModelDocument(QObject *parent = nullptr);
    ~ModelDocument();

signals:
    void contentSet();

public:
    Utils::Result<> open(const Utils::FilePath &filePath,
                         const Utils::FilePath &realFilePath) override;
    bool shouldAutoSave() const override;
    bool isModified() const override;
    bool isSaveAsAllowed() const override;
    Utils::Result<> reload(ReloadFlag flag, ChangeType type) override;

    ExtDocumentController *documentController() const;

    Utils::Result<> load(const Utils::FilePath &fileName);

protected:
    Utils::Result<> saveImpl(const Utils::FilePath &filePath, bool autoSave) override;

private:
    ModelDocumentPrivate *d;
};

} // namespace ModelEditor::Internal
