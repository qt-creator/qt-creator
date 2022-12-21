// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QWidget>

namespace Utils {
class FancyLineEdit;
}

namespace QmlDesigner {

class FilterLineWidget : public QWidget
{
    Q_OBJECT

signals:
    void filterChanged(const QString &filter);

public:
    FilterLineWidget(QWidget *parent = nullptr);

    void clear();

private:
    Utils::FancyLineEdit *m_edit;
};

} // namespace QmlDesigner.
