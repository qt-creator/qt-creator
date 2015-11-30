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

#ifndef ENVIRONMENTASPECTWIDGET_H
#define ENVIRONMENTASPECTWIDGET_H

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
    EnvironmentAspectWidget(EnvironmentAspect *aspect, QWidget *additionalWidget = 0);

    QString displayName() const;
    virtual EnvironmentAspect *aspect() const;

    QWidget *additionalWidget() const;

private slots:
    void baseEnvironmentSelected(int idx);
    void changeBaseEnvironment();
    void userChangesEdited();
    void changeUserChanges(QList<Utils::EnvironmentItem> changes);
    void environmentChanged();

private:
    EnvironmentAspect *m_aspect;
    bool m_ignoreChange;

    QWidget *m_additionalWidget;
    QComboBox *m_baseEnvironmentComboBox;
    Utils::DetailsWidget *m_detailsContainer;
    EnvironmentWidget *m_environmentWidget;
};

} // namespace ProjectExplorer

#endif // ENVIRONMENTASPECTWIDGET_H
