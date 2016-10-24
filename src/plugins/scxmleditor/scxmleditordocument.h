/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    OpenResult open(QString *errorString, const QString &fileName, const QString &realFileName) override;
    bool save(QString *errorString, const QString &fileName, bool autoSave) override;
    bool shouldAutoSave() const override;
    bool isSaveAsAllowed() const override;
    bool isModified() const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

    // Internal
    Common::MainWidget *designWidget() const;
    void syncXmlFromDesignWidget();
    QString designWidgetContents() const;
    void setFilePath(const Utils::FileName&) override;

signals:
    void reloadRequested(QString *errorString, const QString &);

private:
    QPointer<Common::MainWidget> m_designWidget;
};

} // namespace Internal
} // namespace ScxmlEditor
