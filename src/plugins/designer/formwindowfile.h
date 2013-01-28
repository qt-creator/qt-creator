/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <coreplugin/textdocument.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
class QFile;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

class FormWindowFile : public Core::TextDocument
{
    Q_OBJECT

public:
    explicit FormWindowFile(QDesignerFormWindowInterface *form, QObject *parent = 0);

    // IDocument
    virtual bool save(QString *errorString, const QString &fileName, bool autoSave);
    virtual QString fileName() const;
    virtual bool shouldAutoSave() const;
    virtual bool isModified() const;
    virtual bool isSaveAsAllowed() const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;
    virtual QString mimeType() const;
    virtual void rename(const QString &newName);

    // Internal
    void setSuggestedFileName(const QString &fileName);

    bool writeFile(const QString &fileName, QString *errorString) const;

    QDesignerFormWindowInterface *formWindow() const;

signals:
    // Internal
    void saved();
    void reload(QString *errorString, const QString &);
    void setDisplayName(const QString &);

public slots:
    void setFileName(const QString &);
    void setShouldAutoSave(bool sad = true) { m_shouldAutoSave = sad; }

private slots:
    void slotFormWindowRemoved(QDesignerFormWindowInterface *w);

private:
    const QString m_mimeType;

    QString m_fileName;
    QString m_suggestedName;
    bool m_shouldAutoSave;
    // Might actually go out of scope before the IEditor due
    // to deleting the WidgetHost which owns it.
    QPointer<QDesignerFormWindowInterface> m_formWindow;
};

} // namespace Internal
} // namespace Designer

#endif // FORMWINDOWFILE_H
