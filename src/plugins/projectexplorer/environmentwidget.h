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

#include "projectexplorer_export.h"

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace Utils {
class Environment;
class EnvironmentItem;
} // namespace Utils

namespace ProjectExplorer {

class EnvironmentWidgetPrivate;

class PROJECTEXPLORER_EXPORT EnvironmentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EnvironmentWidget(QWidget *parent, QWidget *additionalDetailsWidget = nullptr);
    ~EnvironmentWidget() override;

    void setBaseEnvironmentText(const QString &text);
    void setBaseEnvironment(const Utils::Environment &env);

    QList<Utils::EnvironmentItem> userChanges() const;
    void setUserChanges(const QList<Utils::EnvironmentItem> &list);

signals:
    void userChangesChanged();
    void detailsVisibleChanged(bool visible);

private:
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

    EnvironmentWidgetPrivate *d;
};

} // namespace ProjectExplorer
