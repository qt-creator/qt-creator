/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef ENVIRONMENTEDITMODEL_H
#define ENVIRONMENTEDITMODEL_H

#include "projectexplorer_export.h"

#include <QtGui/QWidget>

QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace Utils {
class Environment;
class EnvironmentItem;
}

namespace ProjectExplorer {
struct EnvironmentWidgetPrivate;

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
    void environmentCurrentIndexChanged(const QModelIndex &current);
    void invalidateCurrentIndex();
    void updateSummaryText();
    void focusIndex(const QModelIndex &index);
    void updateButtons();

private:
    QScopedPointer<EnvironmentWidgetPrivate> d;
};

} // namespace ProjectExplorer

#endif // ENVIRONMENTEDITMODEL_H
