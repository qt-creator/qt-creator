/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TARGETSELECTOR_H
#define TARGETSELECTOR_H

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
    struct Target {
        QString name;
        int currentSubIndex;
    };

    explicit TargetSelector(QWidget *parent = 0);

    QSize sizeHint() const;

    int targetWidth() const;
    QString runButtonString() const { return tr("Run"); }
    QString buildButtonString() const { return tr("Build"); }

    Target targetAt(int index) const;
    int targetCount() const { return m_targets.size(); }
    int currentIndex() const { return m_currentTargetIndex; }
    int currentSubIndex() const { return m_targets.at(m_currentTargetIndex).currentSubIndex; }

    void setTargetMenu(QMenu *menu);

public:
    void insertTarget(int index, const QString &name);
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
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void leaveEvent(QEvent *event);
    bool event(QEvent *e);

private slots:
    void changeButtonPressed();
    void updateButtons();
    void menuAboutToShow();
    void menuAboutToHide();
private:
    void getControlAt(int x, int y, int *buttonIndex, int *targetIndex, int *targetSubIndex);
    int maxVisibleTargets() const;

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
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TARGETSELECTOR_H
