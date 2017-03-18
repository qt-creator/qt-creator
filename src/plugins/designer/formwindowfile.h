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
    explicit FormWindowFile(QDesignerFormWindowInterface *form, QObject *parent = 0);
    ~FormWindowFile() override { }

    // IDocument
    OpenResult open(QString *errorString, const QString &fileName,
                    const QString &realFileName) override;
    bool save(QString *errorString, const QString &fileName, bool autoSave) override;
    QByteArray contents() const override;
    bool setContents(const QByteArray &contents) override;
    bool shouldAutoSave() const override;
    bool isModified() const override;
    bool isSaveAsAllowed() const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
    QString fallbackSaveAsFileName() const override;

    // Internal
    void setFallbackSaveAsFileName(const QString &fileName);

    bool writeFile(const QString &fileName, QString *errorString) const;

    QDesignerFormWindowInterface *formWindow() const;
    void syncXmlFromFormWindow();
    QString formWindowContents() const;
    ResourceHandler *resourceHandler() const;

    void setFilePath(const Utils::FileName &) override;
    void setShouldAutoSave(bool sad = true) { m_shouldAutoSave = sad; }
    void updateIsModified();

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
