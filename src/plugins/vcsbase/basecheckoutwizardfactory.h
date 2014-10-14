/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASECHECKOUTWIZARDFACTORY_H
#define BASECHECKOUTWIZARDFACTORY_H

#include "vcsbase_global.h"
#include <coreplugin/iwizardfactory.h>

#include <utils/fileutils.h>

#include <functional>

namespace VcsBase {
class BaseCheckoutWizard;
class VcsCommand;

class VCSBASE_EXPORT BaseCheckoutWizardFactory : public Core::IWizardFactory
{
    Q_OBJECT

public:
    BaseCheckoutWizardFactory();

    void runWizard(const QString &path, QWidget *parent, const QString &platform, const QVariantMap &extraValues);

    static QString openProject(const Utils::FileName &path, QString *errorMessage);

    typedef std::function<BaseCheckoutWizard *(const Utils::FileName &path, QWidget *parent)> WizardCreator;
    void setWizardCreator(const WizardCreator &creator);

private:
    WizardCreator m_wizardCreator;
};

} // namespace VcsBase

#endif // BASECHECKOUTWIZARDFACTORY_H
