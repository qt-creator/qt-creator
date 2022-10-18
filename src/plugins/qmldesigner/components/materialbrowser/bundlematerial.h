/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmldesignercorelib_global.h"

#include <QDataStream>
#include <QObject>
#include <QUrl>

namespace QmlDesigner {

class BundleMaterial : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString bundleMaterialName MEMBER m_name CONSTANT)
    Q_PROPERTY(QUrl bundleMaterialIcon MEMBER m_icon CONSTANT)
    Q_PROPERTY(bool bundleMaterialVisible MEMBER m_visible NOTIFY materialVisibleChanged)
    Q_PROPERTY(bool bundleMaterialImported READ imported WRITE setImported NOTIFY materialImportedChanged)

public:
    BundleMaterial(QObject *parent,
                   const QString &name,
                   const QString &qml,
                   const TypeName &type,
                   const QUrl &icon,
                   const QStringList &files);

    bool filter(const QString &searchText);

    QUrl icon() const;
    QString qml() const;
    TypeName type() const;
    QStringList files() const;
    bool visible() const;

    bool setImported(bool imported);
    bool imported() const;

signals:
    void materialVisibleChanged();
    void materialImportedChanged();

private:
    QString m_name;
    QString m_qml;
    TypeName m_type;
    QUrl m_icon;
    QStringList m_files;

    bool m_visible = true;
    bool m_imported = false;
};

} // namespace QmlDesigner
