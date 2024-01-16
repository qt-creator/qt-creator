// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/idocument.h>

namespace ResourceEditor::Internal {

class RelativeResourceModel;
class ResourceEditorW;
class QrcEditor;

class ResourceEditorDocument
  : public Core::IDocument
{
    Q_OBJECT
    Q_PROPERTY(QString plainText READ plainText STORED false) // For access by code pasters

public:
    ResourceEditorDocument(QObject *parent = nullptr);

    //IDocument
    OpenResult open(QString *errorString, const Utils::FilePath &filePath,
                    const Utils::FilePath &realFilePath) override;
    QString plainText() const;
    QByteArray contents() const override;
    bool setContents(const QByteArray &contents) override;
    bool shouldAutoSave() const override;
    bool isModified() const override;
    bool isSaveAsAllowed() const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
    void setFilePath(const Utils::FilePath &newName) override;
    void setBlockDirtyChanged(bool value);

    RelativeResourceModel *model() const;
    void setShouldAutoSave(bool save);

signals:
    void loaded(bool success);

protected:
    bool saveImpl(QString *errorString, const Utils::FilePath &filePath, bool autoSave) override;

private:
    void dirtyChanged(bool);

    RelativeResourceModel *m_model;
    bool m_blockDirtyChanged = false;
    bool m_shouldAutoSave = false;
};

void setupResourceEditor(QObject *guard);

} // ResourceEditor::Internal
