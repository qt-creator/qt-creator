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

#ifndef TARGETSETTINGSWIDGET_H
#define TARGETSETTINGSWIDGET_H

#include "targetselector.h"

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace ProjectExplorer {
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

    bool isAddButtonEnabled() const;
    bool isRemoveButtonEnabled() const;

public:
    void insertTarget(int index, const QString &name);
    void renameTarget(int index, const QString &name);
    void removeTarget(int index);
    void setCurrentIndex(int index);
    void setCurrentSubIndex(int index);
    void setAddButtonEnabled(bool enabled);
    void setAddButtonMenu(QMenu *menu);
    void setRemoveButtonEnabled(bool enabled);

signals:
    void removeButtonClicked();
    void currentChanged(int targetIndex, int subIndex);

protected:
    void resizeEvent(QResizeEvent *);
    void changeEvent(QEvent *e);

private:
    void updateTargetSelector();
    Ui::TargetSettingsWidget *ui;

    TargetSelector *m_targetSelector;
    QWidget *m_shadow;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TARGETSETTINGSWIDGET_H
