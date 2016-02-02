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

#include "environmentaspect.h"
#include "runconfiguration.h"

#include <utils/environment.h>

#include <QList>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace Utils { class DetailsWidget; }

namespace ProjectExplorer {

class EnvironmentWidget;

class PROJECTEXPLORER_EXPORT EnvironmentAspectWidget : public RunConfigWidget
{
    Q_OBJECT

public:
    explicit EnvironmentAspectWidget(EnvironmentAspect *aspect, QWidget *additionalWidget = 0);

    QString displayName() const override;
    virtual EnvironmentAspect *aspect() const;

    QWidget *additionalWidget() const;

private:
    void baseEnvironmentSelected(int idx);
    void changeBaseEnvironment();
    void userChangesEdited();
    void changeUserChanges(QList<Utils::EnvironmentItem> changes);
    void environmentChanged();

    EnvironmentAspect *m_aspect;
    bool m_ignoreChange;

    QWidget *m_additionalWidget;
    QComboBox *m_baseEnvironmentComboBox;
    Utils::DetailsWidget *m_detailsContainer;
    EnvironmentWidget *m_environmentWidget;
};

} // namespace ProjectExplorer
