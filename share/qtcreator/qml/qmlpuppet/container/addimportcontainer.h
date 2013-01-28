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

#ifndef ADDIMPORTCONTAINER_H
#define ADDIMPORTCONTAINER_H

#include <QMetaType>
#include <QUrl>
#include <QString>
#include <QStringList>

namespace QmlDesigner {

class AddImportContainer
{
    friend QDataStream &operator>>(QDataStream &in, AddImportContainer &command);
public:
    AddImportContainer();
    AddImportContainer(const QUrl &url, const QString &fileName, const QString &version, const QString &alias, const QStringList &mportPathList);

    QUrl url() const;
    QString fileName() const;
    QString version() const;
    QString alias() const;
    QStringList importPaths() const;

private:
    QUrl m_url;
    QString m_fileName;
    QString m_version;
    QString m_alias;
    QStringList m_importPathList;
};

QDataStream &operator<<(QDataStream &out, const AddImportContainer &command);
QDataStream &operator>>(QDataStream &in, AddImportContainer &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::AddImportContainer)

#endif // ADDIMPORTCONTAINER_H
