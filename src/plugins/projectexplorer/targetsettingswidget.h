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

#ifndef TARGETSETTINGSWIDGET_H
#define TARGETSETTINGSWIDGET_H

#include "targetselector.h"

QT_BEGIN_NAMESPACE
class QMenu;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Target;
class Kit;
namespace Internal {

namespace Ui {
    class TargetSettingsWidget;
}

class TargetSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TargetSettingsWidget(QWidget *parent = 0);
    ~TargetSettingsWidget();

    void setCentralWidget(QWidget *widget);

    QString targetNameAt(int index) const;
    int targetCount() const;
    int currentIndex() const;
    int currentSubIndex() const;

public:
    void insertTarget(int index, const QString &name);
    void renameTarget(int index, const QString &name);
    void removeTarget(int index);
    void setCurrentIndex(int index);
    void setCurrentSubIndex(int index);
    void setAddButtonEnabled(bool enabled);
    void setAddButtonMenu(QMenu *menu);
    void setTargetMenu(QMenu *menu);
signals:
    void currentChanged(int targetIndex, int subIndex);
    void manageButtonClicked();
    void duplicateButtonClicked();
    void changeKitButtonClicked();
    void toolTipRequested(const QPoint &globalPosition, int targetIndex);
    void menuShown(int targetIndex);

protected:
    void changeEvent(QEvent *e);

private:
    Ui::TargetSettingsWidget *ui;

    TargetSelector *m_targetSelector;
    QPushButton *m_addButton;
    QPushButton *m_manageButton;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TARGETSETTINGSWIDGET_H
