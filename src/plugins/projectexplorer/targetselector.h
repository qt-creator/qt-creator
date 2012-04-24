/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef TARGETSELECTOR_H
#define TARGETSELECTOR_H

#include <QWidget>
#include <QPixmap>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {

class TargetSelector : public QWidget
{
Q_OBJECT
public:
    struct Target {
        QString name;
        int currentSubIndex;
    };

    explicit TargetSelector(QWidget *parent = 0);

    QSize minimumSizeHint() const;

    int targetWidth() const;
    QString runButtonString() const { return tr("Run"); }
    QString buildButtonString() const { return tr("Build"); }

    Target targetAt(int index) const;
    int targetCount() const { return m_targets.size(); }
    int currentIndex() const { return m_currentTargetIndex; }
    int currentSubIndex() const { return m_targets.at(m_currentTargetIndex).currentSubIndex; }

    bool isAddButtonEnabled() const;
    bool isRemoveButtonEnabled() const;

public:
    void insertTarget(int index, const QString &name);
    void renameTarget(int index, const QString &name);
    void removeTarget(int index);
    void setCurrentIndex(int index);
    void setCurrentSubIndex(int subindex);
    void setAddButtonEnabled(bool enabled);
    void setRemoveButtonEnabled(bool enabled);
    void setAddButtonMenu(QMenu *menu);

signals:
    void removeButtonClicked();
    // This signal is emitted whenever the target pointed to by the indices
    // has changed.
    void currentChanged(int targetIndex, int subIndex);

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    const QImage m_unselected;
    const QImage m_runselected;
    const QImage m_buildselected;
    const QPixmap m_targetaddbutton;
    const QPixmap m_targetaddbuttondisabled;
    const QPixmap m_targetremovebutton;
    const QPixmap m_targetremovebuttondisabled;

    QList<Target> m_targets;

    int m_currentTargetIndex;
    bool m_addButtonEnabled;
    bool m_removeButtonEnabled;

    QMenu *m_addButtonMenu;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TARGETSELECTOR_H
