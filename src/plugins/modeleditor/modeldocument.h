// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/idocument.h>

namespace qmt { class Uid; }

namespace ModelEditor {
namespace Internal {

class ExtDocumentController;

class ModelDocument :
        public Core::IDocument
{
    Q_OBJECT
    class ModelDocumentPrivate;

public:
    explicit ModelDocument(QObject *parent = nullptr);
    ~ModelDocument();

signals:
    void contentSet();

public:
    OpenResult open(QString *errorString,
                    const Utils::FilePath &filePath,
                    const Utils::FilePath &realFilePath) override;
    bool shouldAutoSave() const override;
    bool isModified() const override;
    bool isSaveAsAllowed() const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

    ExtDocumentController *documentController() const;

    OpenResult load(QString *errorString, const QString &fileName);

protected:
    bool saveImpl(QString *errorString, const Utils::FilePath &filePath, bool autoSave) override;

private:
    ModelDocumentPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
