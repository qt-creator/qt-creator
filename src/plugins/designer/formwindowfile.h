/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef FORMWINDOWFILE_H
#define FORMWINDOWFILE_H

#include <coreplugin/ifile.h>

#include "widgethost.h"
#include "designerconstants.h"

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
class QFile;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

class FormWindowSelection;

class FormWindowFile : public Core::IFile
{
    Q_OBJECT

public:
    FormWindowFile(QDesignerFormWindowInterface *form, QObject *parent = 0);

    // IFile
    bool save(const QString &fileName = QString());
    QString fileName() const;
    bool isModified() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;
    void modified(Core::IFile::ReloadBehavior *behavior);
    QString defaultPath() const;
    QString suggestedFileName() const;
    virtual QString mimeType() const;

    // Internal
    void setSuggestedFileName(const QString &fileName);
    bool writeFile(const QString &fileName, QString &errorString) const;
    bool writeFile(QFile &file, QString &errorString) const;

signals:
    // IFile
    void changed();
    // Internal
    void reload(const QString &);
    void setDisplayName(const QString &);

public slots:
    void setFileName(const QString &);

private:
    const QString m_mimeType;
    QString m_fileName;
    QString m_suggestedName;

    QDesignerFormWindowInterface *m_formWindow;
};

} // namespace Internal
} // namespace Designer

#endif // FORMWINDOWFILE_H
