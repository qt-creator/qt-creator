/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
