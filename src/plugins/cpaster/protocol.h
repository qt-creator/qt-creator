/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QtCore/QObject>

namespace Core {
    class IOptionsPage;
}

namespace CodePaster {
class Protocol : public QObject
{
    Q_OBJECT
public:
    Protocol();
    virtual ~Protocol();

    virtual QString name() const = 0;

    bool canFetch() const;
    bool canPost() const;
    virtual bool canList() const = 0;
    virtual bool hasSettings() const;
    virtual Core::IOptionsPage* settingsPage();

    virtual void fetch(const QString &id) = 0;
    virtual void list();
    virtual void paste(const QString &text,
                       const QString &username = QString(),
                       const QString &comment = QString(),
                       const QString &description = QString()) = 0;

signals:
    void pasteDone(const QString &link);
    void fetchDone(const QString &titleDescription,
                   const QString &content,
                   bool error);
    void listDone(const QString &name, const QStringList &result);
};

} //namespace CodePaster

#endif // PROTOCOL_H
