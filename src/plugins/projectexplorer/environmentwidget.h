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

#ifndef ENVIRONMENTWIDGET_H
#define ENVIRONMENTWIDGET_H

#include "projectexplorer_export.h"

#include <QWidget>
#include <QTreeView>

QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace Utils {
class Environment;
class EnvironmentItem;
}

namespace ProjectExplorer {
namespace Internal {
class EnvironmentTreeView : public QTreeView
{
public:
    EnvironmentTreeView(QWidget *parent);
protected:
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    void keyPressEvent(QKeyEvent *event);
};
}

class EnvironmentWidgetPrivate;

class PROJECTEXPLORER_EXPORT EnvironmentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EnvironmentWidget(QWidget *parent, QWidget *additionalDetailsWidget = 0);
    virtual ~EnvironmentWidget();

    void setBaseEnvironmentText(const QString &text);
    void setBaseEnvironment(const Utils::Environment &env);

    QList<Utils::EnvironmentItem> userChanges() const;
    void setUserChanges(const QList<Utils::EnvironmentItem> &list);

signals:
    void userChangesChanged();
    void detailsVisibleChanged(bool visible);

private slots:
    void editEnvironmentButtonClicked();
    void addEnvironmentButtonClicked();
    void removeEnvironmentButtonClicked();
    void unsetEnvironmentButtonClicked();
    void batchEditEnvironmentButtonClicked();
    void environmentCurrentIndexChanged(const QModelIndex &current);
    void invalidateCurrentIndex();
    void updateSummaryText();
    void focusIndex(const QModelIndex &index);
    void updateButtons();
    void linkActivated(const QString &link);

private:
    EnvironmentWidgetPrivate *d;
};

} // namespace ProjectExplorer

#endif // ENVIRONMENTWIDGET_H
