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

#include <QWidget>
#include <QPixmap>

QT_BEGIN_NAMESPACE
class QMenu;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {
class QPixmapButton;

class TargetSelector : public QWidget
{
Q_OBJECT
public:
    class Target {
    public:
        QString name;
        int currentSubIndex;
    };

    explicit TargetSelector(QWidget *parent = 0);

    QSize sizeHint() const override;

    int targetWidth() const;
    QString runButtonString() const { return tr("Run"); }
    QString buildButtonString() const { return tr("Build"); }

    Target targetAt(int index) const;
    int targetCount() const { return m_targets.size(); }
    int currentIndex() const { return m_currentTargetIndex; }
    int currentSubIndex() const {
        return m_currentTargetIndex == -1 ? -1
                                          : m_targets.at(m_currentTargetIndex).currentSubIndex;
    }

    void setTargetMenu(QMenu *menu);

public:
    void insertTarget(int index, int subIndex, const QString &name);
    void renameTarget(int index, const QString &name);
    void removeTarget(int index);
    void setCurrentIndex(int index);
    void setCurrentSubIndex(int subindex);

signals:
    // This signal is emitted whenever the target pointed to by the indices
    // has changed.
    void currentChanged(int targetIndex, int subIndex);
    void toolTipRequested(const QPoint &globalPosition, int targetIndex);
    void menuShown(int targetIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *e) override;

private:
    void changeButtonPressed();
    void updateButtons();
    void menuAboutToShow();
    void menuAboutToHide();
    void getControlAt(int x, int y, int *buttonIndex, int *targetIndex, int *targetSubIndex);
    int maxVisibleTargets() const;
    void ensureCurrentIndexVisible();

    const QImage m_unselected;
    const QImage m_runselected;
    const QImage m_buildselected;
    const QPixmap m_targetRightButton;
    const QPixmap m_targetLeftButton;
    const QPixmap m_targetChangePixmap;
    const QPixmap m_targetChangePixmap2;

    QPixmapButton *m_targetChangeButton;

    QList<Target> m_targets;

    int m_currentTargetIndex;
    int m_currentHoveredTargetIndex;
    int m_startIndex;
    bool m_menuShown;
    mutable bool m_targetWidthNeedsUpdate;
};

} // namespace Internal
} // namespace ProjectExplorer
