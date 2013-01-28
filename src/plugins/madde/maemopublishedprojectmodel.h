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

#ifndef MAEMOPUBLISHEDPROJECTMODEL_H
#define MAEMOPUBLISHEDPROJECTMODEL_H

#include <QSet>
#include <QStringList>
#include <QFileSystemModel>

namespace Madde {
namespace Internal {

class MaemoPublishedProjectModel : public QFileSystemModel
{
    Q_OBJECT
public:
    explicit MaemoPublishedProjectModel(QObject *parent = 0);
    void initFilesToExclude();
    QStringList filesToExclude() const { return m_filesToExclude.toList(); }

private:
    virtual int columnCount(const QModelIndex &parent) const;
    virtual int rowCount(const QModelIndex &parent) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
        int role) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value,
        int role);
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    void initFilesToExclude(const QString &filePath);

    QSet<QString> m_filesToExclude;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOPUBLISHEDPROJECTMODEL_H
