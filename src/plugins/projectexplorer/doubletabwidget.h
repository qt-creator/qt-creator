/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DOUBLETABWIDGET_H
#define DOUBLETABWIDGET_H

#include <QVector>
#include <QWidget>
#include <QPixmap>

namespace ProjectExplorer {
namespace Internal {

namespace Ui { class DoubleTabWidget; }

class DoubleTabWidget : public QWidget
{
    Q_OBJECT
public:
    DoubleTabWidget(QWidget *parent = 0);
    ~DoubleTabWidget();

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
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    bool event(QEvent *event);
    QSize minimumSizeHint() const;

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

    const QPixmap m_left;
    const QPixmap m_mid;
    const QPixmap m_right;

    Ui::DoubleTabWidget *ui;


    QString m_title;
    QList<Tab> m_tabs;
    int m_currentIndex;
    QVector<int> m_currentTabIndices;
    int m_lastVisibleIndex;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // DOUBLETABWIDGET_H
