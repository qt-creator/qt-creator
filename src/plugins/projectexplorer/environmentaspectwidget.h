// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "environmentaspect.h"
#include "runconfiguration.h"

#include <utils/environment.h>
#include <utils/guard.h>

#include <QList>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QComboBox;
class QHBoxLayout;
QT_END_NAMESPACE

namespace Utils { class DetailsWidget; }

namespace ProjectExplorer {

class EnvironmentWidget;

class PROJECTEXPLORER_EXPORT EnvironmentAspectWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EnvironmentAspectWidget(EnvironmentAspect *aspect);

protected:
    EnvironmentAspect *aspect() const { return m_aspect; }
    EnvironmentWidget *envWidget() const { return m_environmentWidget; }
    void addWidget(QWidget *widget);

private:
    void baseEnvironmentSelected(int idx);
    void changeBaseEnvironment();
    void userChangesEdited();
    void changeUserChanges(Utils::EnvironmentItems changes);
    void environmentChanged();

    EnvironmentAspect *m_aspect;
    Utils::Guard m_ignoreChanges;
    QHBoxLayout *m_baseLayout = nullptr;
    QComboBox *m_baseEnvironmentComboBox = nullptr;
    EnvironmentWidget *m_environmentWidget = nullptr;
};

} // namespace ProjectExplorer
