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

#ifndef FILESHAREPROTOCOL_H
#define FILESHAREPROTOCOL_H

#include "protocol.h"

#include <QSharedPointer>

namespace CodePaster {

class FileShareProtocolSettingsPage;
struct FileShareProtocolSettings;

/* FileShareProtocol: Allows for pasting via a shared network
 * drive by writing XML files. */

class FileShareProtocol : public Protocol
{
    Q_OBJECT

public:
    FileShareProtocol();
    virtual ~FileShareProtocol();

    virtual QString name() const;
    virtual unsigned capabilities() const;
    virtual bool hasSettings() const;
    virtual Core::IOptionsPage *settingsPage() const;

    virtual bool checkConfiguration(QString *errorMessage = 0);
    virtual void fetch(const QString &id);
    virtual void list();
    virtual void paste(const QString &text,
                       ContentType ct = Text,
                       const QString &username = QString(),
                       const QString &comment = QString(),
                       const QString &description = QString());

private:
    const QSharedPointer<FileShareProtocolSettings> m_settings;
    FileShareProtocolSettingsPage *m_settingsPage;
};

} // namespace CodePaster

#endif // FILESHAREPROTOCOL_H
