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

#include <utils/environmentfwd.h>
#include <utils/namevalueitem.h>

#include <QWidget>

#include <functional>
#include <memory>

QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace ProjectExplorer {

class EnvironmentWidgetPrivate;

class PROJECTEXPLORER_EXPORT EnvironmentWidget : public QWidget
{
    Q_OBJECT

public:
    enum Type { TypeLocal, TypeRemote };
    explicit EnvironmentWidget(QWidget *parent, Type type,
                               QWidget *additionalDetailsWidget = nullptr);
    ~EnvironmentWidget() override;

    void setBaseEnvironmentText(const QString &text);
    void setBaseEnvironment(const Utils::Environment &env);

    Utils::EnvironmentItems userChanges() const;
    void setUserChanges(const Utils::EnvironmentItems &list);

    using OpenTerminalFunc = std::function<void(const Utils::Environment &env)>;
    void setOpenTerminalFunc(const OpenTerminalFunc &func);

    void expand();

signals:
    void userChangesChanged();
    void detailsVisibleChanged(bool visible);

private:
    void editEnvironmentButtonClicked();
    void addEnvironmentButtonClicked();
    void removeEnvironmentButtonClicked();
    void unsetEnvironmentButtonClicked();
    void appendPathButtonClicked();
    void prependPathButtonClicked();
    void batchEditEnvironmentButtonClicked();
    void environmentCurrentIndexChanged(const QModelIndex &current);
    void invalidateCurrentIndex();
    void updateSummaryText();
    void focusIndex(const QModelIndex &index);
    void updateButtons();
    void linkActivated(const QString &link);
    bool currentEntryIsPathList(const QModelIndex &current) const;

    using PathListModifier = std::function<QString(const QString &oldList, const QString &newDir)>;
    void amendPathList(Utils::NameValueItem::Operation op);

    const std::unique_ptr<EnvironmentWidgetPrivate> d;
};

} // namespace ProjectExplorer
