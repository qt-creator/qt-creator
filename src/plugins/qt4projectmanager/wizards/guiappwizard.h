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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef GUIAPPWIZARD_H
#define GUIAPPWIZARD_H

#include "qtwizard.h"

namespace Qt4ProjectManager {
namespace Internal {

struct GuiAppParameters;

class GuiAppWizard : public QtWizard
{
    Q_DISABLE_COPY(GuiAppWizard)
    Q_OBJECT

public:
    GuiAppWizard();

protected:
    GuiAppWizard(const QString &id,
                 const QString &category,
                 const QString &categoryTranslationScope,
                 const QString &displayCategory,
                 const QString &name,
                 const QString &description,
                 const QIcon &icon,
                 bool createMobile);
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;

private:
    static bool parametrizeTemplate(const QString &templatePath, const QString &templateName,
                                    const GuiAppParameters &params,
                                    QString *target, QString *errorMessage);

    bool m_createMobileProject;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // GUIAPPWIZARD_H
