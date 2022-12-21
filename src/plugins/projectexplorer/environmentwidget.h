// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    using PathListModifier = std::function<QString(const QString &oldList, const QString &newDir)>;
    void amendPathList(Utils::NameValueItem::Operation op);

    const std::unique_ptr<EnvironmentWidgetPrivate> d;
};

} // namespace ProjectExplorer
