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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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

QDebug operator <<(QDebug debug, const AddImportContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::AddImportContainer)

#endif // ADDIMPORTCONTAINER_H
