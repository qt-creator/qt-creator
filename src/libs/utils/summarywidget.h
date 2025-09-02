// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "detailswidget.h"
#include "result.h"
#include "infolabel.h"

#include <QWidget>

namespace Utils {

class QTCREATOR_UTILS_EXPORT SummaryWidget : public QWidget
{
public:
    SummaryWidget(const QMap<int, QString> &validationPoints, const QString &validText,
                  const QString &invalidText, DetailsWidget *detailsWidget);

    template<class T>
    void setPointValid(int key, const Result<T> &test)
    {
        setPointValid(key, test.has_value(), test.has_value() ? QString{} : test.error());
    }

    void setPointValid(int key, bool valid, const QString errorText = {});
    bool rowsOk(const QList<int> &keys) const;
    bool allRowsOk() const;
    void setInfoText(const QString &text);
    void setInProgressText(const QString &text);
    void setSetupOk(bool ok);

private:
    void updateUi();

    class RowData {
    public:
        InfoLabel *m_infoLabel = nullptr;
        bool m_valid = false;
        QString m_validText;
    };

    QString m_validText;
    QString m_invalidText;
    QString m_infoText;
    DetailsWidget *m_detailsWidget = nullptr;
    QMap<int, RowData> m_validationData;
};

} // namespace Utils
