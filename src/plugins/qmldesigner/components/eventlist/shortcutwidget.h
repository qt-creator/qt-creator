/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Event List module.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
******************************************************************************/
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
