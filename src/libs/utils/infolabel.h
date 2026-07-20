// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "elidinglabel.h"

namespace Utils {

enum class InfoLabelType {
    Information,
    Warning,
    Error,
    Ok,
    NotOk,
    None
};

class QTCREATOR_UTILS_EXPORT InfoLabel : public ElidingLabel
{
public:
    explicit InfoLabel(QWidget *parent);
    explicit InfoLabel(const QString &text = {}, InfoLabelType type = InfoLabelType::Information,
                       QWidget *parent = nullptr);

    InfoLabelType type() const;
    void setType(InfoLabelType type);
    bool filled() const;
    void setFilled(bool filled);
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    InfoLabelType m_type = InfoLabelType::Information;
    bool m_filled = false;
};

} // namespace Utils
