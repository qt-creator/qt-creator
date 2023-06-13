// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbasesubmiteditor.h"

#include <coreplugin/idocument.h>

namespace VcsBase {
class VcsBaseSubmitEditor;

namespace Internal {

class SubmitEditorFile : public Core::IDocument
{
public:
    explicit SubmitEditorFile(VcsBaseSubmitEditor *editor);

    OpenResult open(QString *errorString, const Utils::FilePath &filePath,
                    const Utils::FilePath &realFilePath) override;
    QByteArray contents() const override;
    bool setContents(const QByteArray &contents) override;

    bool isModified() const override { return m_modified; }
    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;

    void setModified(bool modified = true);

protected:
    bool saveImpl(QString *errorString, const Utils::FilePath &filePath, bool autoSave) override;

private:
    bool m_modified;
    VcsBaseSubmitEditor *m_editor;
};

} // namespace Internal
} // namespace VcsBase
