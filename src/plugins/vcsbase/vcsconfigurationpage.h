/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef VCSCONFIGURATIONPAGE_H
#define VCSCONFIGURATIONPAGE_H

#include "vcsbase_global.h"

#include <QtGui/QWizardPage>

namespace Core {
class IVersionControl;
} // namespace Core

namespace VCSBase {

class VcsConfigurationPagePrivate;

class VCSBASE_EXPORT VcsConfigurationPage : public QWizardPage {
    Q_OBJECT

public:
    explicit VcsConfigurationPage(const Core::IVersionControl *, QWidget *parent = 0);
    ~VcsConfigurationPage();

    bool isComplete() const;

private slots:
    void openConfiguration();
private:
    VcsConfigurationPagePrivate *const m_d;
};

} // namespace VCSBase

#endif // VCSCONFIGURATIONPAGE_H
