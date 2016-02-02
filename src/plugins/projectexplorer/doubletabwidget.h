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

#include <QVector>
#include <QWidget>
#include <QImage>

namespace ProjectExplorer {
namespace Internal {

namespace Ui { class DoubleTabWidget; }

class DoubleTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DoubleTabWidget(QWidget *parent = 0);
    ~DoubleTabWidget() override;

    void setTitle(const QString &title);
    QString title() const { return m_title; }

    void addTab(const QString &name, const QString &fullName, const QStringList &subTabs);
    void insertTab(int index, const QString &name, const QString &fullName, const QStringList &subTabs);
    void removeTab(int index);
    int tabCount() const;

    int currentIndex() const;
    void setCurrentIndex(int index);
    void setCurrentIndex(int index, int subIndex);

    int currentSubIndex() const;

    QStringList subTabs(int index) const;
    void setSubTabs(int index, const QStringList &subTabs);

signals:
    void currentIndexChanged(int index, int subIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;
    QSize minimumSizeHint() const override;

private:
    struct Tab {
        QString name;
        QString fullName;
        bool nameIsUnique;
        QStringList subTabs;
        int currentSubTab;
        QString displayName() const;
    };
    void updateNameIsUniqueAdd(Tab *tab);
    void updateNameIsUniqueRemove(const Tab &tab);

    enum HitArea { HITNOTHING, HITOVERFLOW, HITTAB, HITSUBTAB };
    QPair<DoubleTabWidget::HitArea, int> convertPosToTab(QPoint pos);

    const QImage m_selection;

    Ui::DoubleTabWidget *ui;


    QString m_title;
    QList<Tab> m_tabs;
    int m_currentIndex;
    QVector<int> m_currentTabIndices;
    int m_lastVisibleIndex;
};

} // namespace Internal
} // namespace ProjectExplorer
