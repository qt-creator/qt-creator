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

#include "qtwizard.h"

namespace QmakeProjectManager {
namespace Internal {

struct QtProjectParameters;
class FilesPage;

// Additional parameters required besides QtProjectParameters
struct GuiAppParameters
{
    GuiAppParameters();
    QString className;
    QString baseClassName;
    QString sourceFileName;
    QString headerFileName;
    QString formFileName;
    int widgetWidth;
    int widgetHeight;
    bool designerForm;
    bool isMobileApplication;
};

class GuiAppWizardDialog : public BaseQmakeProjectWizardDialog
{
    Q_OBJECT

public:
    explicit GuiAppWizardDialog(const Core::BaseFileWizardFactory *factory, const QString &templateName,
                                const QIcon &icon,
                                QWidget *parent,
                                const Core::WizardDialogParameters &parameters);

    void setBaseClasses(const QStringList &baseClasses);
    void setSuffixes(const QString &header, const QString &source,  const QString &form);
    void setLowerCaseFiles(bool l);

    QtProjectParameters projectParameters() const;
    GuiAppParameters parameters() const;

private:
    FilesPage *m_filesPage;
};

} // namespace Internal
} // namespace QmakeProjectManager
