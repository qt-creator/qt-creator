// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textdocument.h>
#include <utils/guard.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

class ResourceHandler;

class FormWindowFile : public TextEditor::TextDocument
{
    Q_OBJECT

public:
    explicit FormWindowFile(QDesignerFormWindowInterface *form, QObject *parent = nullptr);
    ~FormWindowFile() override { }

    // IDocument
    OpenResult open(QString *errorString, const Utils::FilePath &filePath,
                    const Utils::FilePath &realFilePath) override;
    QByteArray contents() const override;
    bool setContents(const QByteArray &contents) override;
    bool shouldAutoSave() const override;
    bool isModified() const override;
    bool isSaveAsAllowed() const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
    QString fallbackSaveAsFileName() const override;
    bool supportsCodec(const QTextCodec *codec) const override;

    // Internal
    void setFallbackSaveAsFileName(const QString &fileName);

    bool writeFile(const Utils::FilePath &filePath, QString *errorString) const;

    QDesignerFormWindowInterface *formWindow() const;
    void syncXmlFromFormWindow();
    QString formWindowContents() const;
    ResourceHandler *resourceHandler() const;

    void setFilePath(const Utils::FilePath &) override;
    void setShouldAutoSave(bool sad = true) { m_shouldAutoSave = sad; }
    void updateIsModified();

protected:
    bool saveImpl(QString *errorString, const Utils::FilePath &filePath, bool autoSave) override;

private:
    void slotFormWindowRemoved(QDesignerFormWindowInterface *w);

    QString m_suggestedName;
    bool m_shouldAutoSave = false;
    // Might actually go out of scope before the IEditor due
    // to deleting the WidgetHost which owns it.
    QPointer<QDesignerFormWindowInterface> m_formWindow;
    bool m_isModified = false;
    ResourceHandler *m_resourceHandler = nullptr;
    Utils::Guard m_modificationChangedGuard;
};

} // namespace Internal
} // namespace Designer
