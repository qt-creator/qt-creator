// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QImage>
#include <QPointer>
#include <QWidget>

#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace Welcome {
namespace Internal {

struct Item
{
    QString pointerAnchorObjectName;
    QString title;
    QString brief;
    QString description;
};

class IntroductionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit IntroductionWidget(QWidget *parent = nullptr);

    static void askUserAboutIntroduction(QWidget *parent);

protected:
    bool event(QEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void paintEvent(QPaintEvent *ev) override;
    void keyPressEvent(QKeyEvent *ke) override;
    void mouseReleaseEvent(QMouseEvent *me) override;

private:
    void finish();
    void step();
    void setStep(uint index);
    void resizeToParent();

    QWidget *m_textWidget;
    QLabel *m_stepText;
    QLabel *m_continueLabel;
    QImage m_borderImage;
    QString m_bodyCss;
    std::vector<Item> m_items;
    QPointer<QWidget> m_stepPointerAnchor;
    uint m_step = 0;
};

} // namespace Internal
} // namespace Welcome
