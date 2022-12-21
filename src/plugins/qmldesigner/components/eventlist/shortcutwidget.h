// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <array>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace QmlDesigner {

class ShortcutWidget : public QWidget
{
    Q_OBJECT

signals:
    void done();
    void cancel();

public:
    ShortcutWidget(QWidget *parent = nullptr);
    bool containsFocus() const;
    QString text() const;

    void reset();
    void recordKeysequence(QKeyEvent *event);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QLineEdit *m_text;
    QPushButton *m_button;
    std::array<int, 4> m_key;
    int m_keyNum = 0;
};

} // namespace QmlDesigner.
