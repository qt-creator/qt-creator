// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/idocument.h>

#include <QList>

namespace Squish {
namespace Internal {

class ObjectsMapModel;

class ObjectsMapDocument : public Core::IDocument
{
    Q_OBJECT
public:
    ObjectsMapDocument();

    OpenResult open(QString *errorString,
                    const Utils::FilePath &fileName,
                    const Utils::FilePath &realFileName) override;
    Utils::FilePath fallbackSaveAsPath() const override;
    QString fallbackSaveAsFileName() const override;
    bool isModified() const override { return m_isModified; }
    void setModified(bool modified);
    bool isSaveAsAllowed() const override { return true; }
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

    bool shouldAutoSave() const override { return true; }
    bool setContents(const QByteArray &contents) override;
    QByteArray contents() const override;
    ObjectsMapModel *model() const { return m_contentModel; }

protected:
    bool saveImpl(QString *errorString, const Utils::FilePath &fileName, bool autoSave) override;

private:
    OpenResult openImpl(QString *error,
                        const Utils::FilePath &fileName,
                        const Utils::FilePath &realFileName);
    bool buildObjectsMapTree(const QByteArray &contents);
    bool writeFile(const Utils::FilePath &fileName) const;
    void syncXMLFromEditor();

    ObjectsMapModel *m_contentModel;
    bool m_isModified;
};

} // namespace Internal
} // namespace Squish
