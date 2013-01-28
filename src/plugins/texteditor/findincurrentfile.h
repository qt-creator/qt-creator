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

#ifndef FINDINCURRENTFILE_H
#define FINDINCURRENTFILE_H

#include "basefilefind.h"

#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QPointer>

namespace TextEditor {
namespace Internal {

class FindInCurrentFile : public BaseFileFind
{
    Q_OBJECT

public:
    FindInCurrentFile();

    QString id() const;
    QString displayName() const;
    bool isEnabled() const;
    void writeSettings(QSettings *settings);
    void readSettings(QSettings *settings);

protected:
    Utils::FileIterator *files(const QStringList &nameFilters,
                               const QVariant &additionalParameters) const;
    QVariant additionalParameters() const;
    QString label() const;
    QString toolTip() const;

private slots:
    void handleFileChange(Core::IEditor *editor);

private:
    QPointer<Core::IDocument> m_currentDocument;
};

} // namespace Internal
} // namespace TextEditor

#endif // FINDINCURRENTFILE_H
