// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "summarywidget.h"

#include "qtcassert.h"
#include "utilsicons.h"

#include <QVBoxLayout>

namespace Utils {

SummaryWidget::SummaryWidget(const QMap<int, QString> &validationPoints, const QString &validText,
                             const QString &invalidText, DetailsWidget *detailsWidget)
    : QWidget(detailsWidget)
    , m_validText(validText)
    , m_invalidText(invalidText)
    , m_detailsWidget(detailsWidget)
{
    QTC_CHECK(m_detailsWidget);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(22, 0, 0, 12);
    layout->setSpacing(4);
    for (auto itr = validationPoints.cbegin(); itr != validationPoints.cend(); ++itr) {
        RowData data;
        data.m_infoLabel = new InfoLabel(itr.value());
        data.m_validText = itr.value();
        layout->addWidget(data.m_infoLabel);
        m_validationData[itr.key()] = data;
        setPointValid(itr.key(), false);
    }
    m_detailsWidget->setWidget(this);
    setContentsMargins(0, 0, 0, 0);
}

void SummaryWidget::setPointValid(int key, bool valid, const QString errorText)
{
    if (!m_validationData.contains(key))
        return;
    RowData &data = m_validationData[key];
    data.m_valid = valid;
    data.m_infoLabel->setType(valid ? InfoLabel::Ok : InfoLabel::NotOk);
    data.m_infoLabel->setText(valid || errorText.isEmpty() ? data.m_validText : errorText);
    updateUi();
}

bool SummaryWidget::rowsOk(const QList<int> &keys) const
{
    for (auto key : keys) {
        if (!m_validationData[key].m_valid)
            return false;
    }
    return true;
}

bool SummaryWidget::allRowsOk() const
{
    return rowsOk(m_validationData.keys());
}

void SummaryWidget::setInfoText(const QString &text)
{
    m_infoText = text;
    updateUi();
}

void SummaryWidget::setInProgressText(const QString &text)
{
    m_detailsWidget->setIcon({});
    m_detailsWidget->setSummaryText(QString("%1...").arg(text));
    m_detailsWidget->setState(DetailsWidget::Collapsed);
}

void SummaryWidget::setSetupOk(bool ok)
{
    m_detailsWidget->setState(ok ? DetailsWidget::Collapsed : DetailsWidget::Expanded);
}

void SummaryWidget::updateUi()
{
    bool ok = allRowsOk();
    m_detailsWidget->setIcon(ok ? Icons::OK.icon() : Icons::CRITICAL.icon());
    m_detailsWidget->setSummaryText(ok ? QString("%1 %2").arg(m_validText).arg(m_infoText)
                                       : m_invalidText);
}

} // namespace Utils
