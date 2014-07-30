/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FORMWINDOWFILE_H
#define FORMWINDOWFILE_H

#include <texteditor/basetextdocument.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

class FormWindowFile : public TextEditor::BaseTextDocument
{
    Q_OBJECT

public:
    explicit FormWindowFile(QDesignerFormWindowInterface *form, QObject *parent = 0);
    ~FormWindowFile() { }

    // IDocument
    bool save(QString *errorString, const QString &fileName, bool autoSave);
    bool setContents(const QByteArray &contents);
    bool shouldAutoSave() const;
    bool isModified() const;
    bool isSaveAsAllowed() const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
    QString defaultPath() const;
    QString suggestedFileName() const;

    // Internal
    void setSuggestedFileName(const QString &fileName);

    bool writeFile(const QString &fileName, QString *errorString) const;

    QDesignerFormWindowInterface *formWindow() const;
    void syncXmlFromFormWindow();
    QString formWindowContents() const;

signals:
    // Internal
    void reloadRequested(QString *errorString, const QString &);

public slots:
    void setFilePath(const QString &);
    void setShouldAutoSave(bool sad = true) { m_shouldAutoSave = sad; }
    void updateIsModified();

private slots:
    void slotFormWindowRemoved(QDesignerFormWindowInterface *w);

private:
    QString m_suggestedName;
    bool m_shouldAutoSave;
    // Might actually go out of scope before the IEditor due
    // to deleting the WidgetHost which owns it.
    QPointer<QDesignerFormWindowInterface> m_formWindow;
    bool m_isModified;
};

} // namespace Internal
} // namespace Designer

#endif // FORMWINDOWFILE_H
