// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <texteditor/textdocument.h>

#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QDesignerFormWindowInterface)

namespace ScxmlEditor {

namespace Common {
class MainWidget;
} // namespace Common

namespace Internal {

class ScxmlEditorDocument : public TextEditor::TextDocument
{
    Q_OBJECT

public:
    explicit ScxmlEditorDocument(Common::MainWidget *designWidget, QObject *parent = nullptr);

    // IDocument
    OpenResult open(QString *errorString,
                    const Utils::FilePath &filePath,
                    const Utils::FilePath &realFilePath) override;
    bool shouldAutoSave() const override;
    bool isSaveAsAllowed() const override;
    bool isModified() const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
    bool supportsCodec(const QTextCodec *codec) const override;

    // Internal
    Common::MainWidget *designWidget() const;
    void syncXmlFromDesignWidget();
    QString designWidgetContents() const;
    void setFilePath(const Utils::FilePath&) override;

signals:
    void reloadRequested(QString *errorString, const QString &);

protected:
    bool saveImpl(QString *errorString, const Utils::FilePath &filePath, bool autoSave) override;

private:
    QPointer<Common::MainWidget> m_designWidget;
};

} // namespace Internal
} // namespace ScxmlEditor
