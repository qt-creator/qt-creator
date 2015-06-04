/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FORMWINDOWFILE_H
#define FORMWINDOWFILE_H

#include <texteditor/textdocument.h>

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
    bool setContents(const QByteArray &contents) override;
    bool shouldAutoSave() const override;
    bool isModified() const override;
    bool isSaveAsAllowed() const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
    QString defaultPath() const override;
    QString suggestedFileName() const override;

    // Internal
    void setSuggestedFileName(const QString &fileName);

    bool writeFile(const QString &fileName, QString *errorString) const;

    QDesignerFormWindowInterface *formWindow() const;
    void syncXmlFromFormWindow();
    QString formWindowContents() const;
    ResourceHandler *resourceHandler() const;

public slots:
    void setFilePath(const Utils::FileName &) override;
    void setShouldAutoSave(bool sad = true) { m_shouldAutoSave = sad; }
    void updateIsModified();

private slots:
    void slotFormWindowRemoved(QDesignerFormWindowInterface *w);

private:
    QString m_suggestedName;
    bool m_shouldAutoSave = false;
    // Might actually go out of scope before the IEditor due
    // to deleting the WidgetHost which owns it.
    QPointer<QDesignerFormWindowInterface> m_formWindow;
    bool m_isModified = false;
    ResourceHandler *m_resourceHandler = nullptr;
};

} // namespace Internal
} // namespace Designer

#endif // FORMWINDOWFILE_H
