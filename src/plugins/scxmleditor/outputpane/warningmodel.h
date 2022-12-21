// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
