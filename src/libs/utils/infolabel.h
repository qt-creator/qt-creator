// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "elidinglabel.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT InfoLabel : public ElidingLabel
{
public:
    enum InfoType {
        Information,
        Warning,
        Error,
        Ok,
        NotOk,
        None
    };

    explicit InfoLabel(QWidget *parent);
    explicit InfoLabel(const QString &text = {}, InfoType type = Information,
                       QWidget *parent = nullptr);

    InfoType type() const;
    void setType(InfoType type);
    bool filled() const;
    void setFilled(bool filled);
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    InfoType m_type = Information;
    bool m_filled = false;
};

} // namespace Utils
