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

#ifndef UTILS_ENVIRONMENTMODEL_H
#define UTILS_ENVIRONMENTMODEL_H

#include "utils_global.h"

#include <QAbstractTableModel>

namespace Utils {
class Environment;
class EnvironmentItem;

namespace Internal {
class EnvironmentModelPrivate;
} // namespace Internal

class QTCREATOR_UTILS_EXPORT EnvironmentModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit EnvironmentModel(QObject *parent = 0);
    ~EnvironmentModel();

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    QModelIndex addVariable();
    QModelIndex addVariable(const Utils::EnvironmentItem &item);
    void resetVariable(const QString &name);
    void unsetVariable(const QString &name);
    bool canUnset(const QString &name);
    bool canReset(const QString &name);
    QString indexToVariable(const QModelIndex &index) const;
    QModelIndex variableToIndex(const QString &name) const;
    bool changes(const QString &key) const;
    void setBaseEnvironment(const Utils::Environment &env);
    QList<Utils::EnvironmentItem> userChanges() const;
    void setUserChanges(QList<Utils::EnvironmentItem> list);

signals:
    void userChangesChanged();
    /// Hint to the view where it should make sense to focus on next
    // This is a hack since there is no way for a model to suggest
    // the next interesting place to focus on to the view.
    void focusIndex(const QModelIndex &index);

private:
    Internal::EnvironmentModelPrivate *d;
};

} // namespace Utils

#endif // UTILS_ENVIRONMENTMODEL_H
