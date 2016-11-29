/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "warning.h"

#include <QAbstractTableModel>
#include <QIcon>
#include <QPointer>
#include <QTimer>

namespace ScxmlEditor {

namespace OutputPane {

class WarningModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum WarningRole {
        FilterRole = Qt::UserRole + 1
    };

    WarningModel(QObject *parent = nullptr);
    ~WarningModel() override;

    int count(Warning::Severity type) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setShowWarnings(int type, bool show);
    Warning *createWarning(Warning::Severity severity, const QString &type, const QString &reason, const QString &description);
    Warning *getWarning(const QModelIndex &ind);
    Warning *getWarning(int row);

    void clear(bool sendSignal = true);

signals:
    void warningsChanged();
    void countChanged(int count);
    void modelAboutToBeClear();
    void modelCleared();

private:
    void warningDestroyed(QObject *p);
    QString severityName(Warning::Severity severity) const;
    QIcon severityIcon(Warning::Severity severity) const;

    QVector<Warning*> m_warnings;
    QMap<int, bool> m_warningVisibilities;
    int m_oldCount = 0;
    QPointer<QTimer> m_countChecker;
};

} // namespace OutputPane
} // namespace ScxmlEditor
