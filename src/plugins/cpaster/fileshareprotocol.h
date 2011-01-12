/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef FILESHAREPROTOCOL_H
#define FILESHAREPROTOCOL_H

#include "protocol.h"

#include <QtCore/QSharedPointer>

namespace CodePaster {

class FileShareProtocolSettingsPage;
struct FileShareProtocolSettings;

/* FileShareProtocol: Allows for pasting via a shared network
 * drive by writing XML files. */

class FileShareProtocol : public Protocol
{
    Q_OBJECT
    Q_DISABLE_COPY(FileShareProtocol)
public:
    FileShareProtocol();
    virtual ~FileShareProtocol();

    virtual QString name() const;
    virtual unsigned capabilities() const;
    virtual bool hasSettings() const;
    virtual Core::IOptionsPage *settingsPage() const;

    virtual bool checkConfiguration(QString *errorMessage = 0) const;
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
