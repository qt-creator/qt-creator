/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CUSTOMWIDGETWIZARDDIALOG_H
#define CUSTOMWIDGETWIZARDDIALOG_H

#include "../wizards/qtwizard.h"

#include <QSharedPointer>

namespace Qt4ProjectManager {
namespace Internal {

class CustomWidgetWidgetsWizardPage;
class CustomWidgetPluginWizardPage;
struct PluginOptions;
struct FileNamingParameters;

class CustomWidgetWizardDialog : public BaseQt4ProjectWizardDialog
{
    Q_OBJECT
public:
    explicit CustomWidgetWizardDialog(const QString &templateName,
                                      const QIcon &icon,
                                      QWidget *parent,
                                      const Core::WizardDialogParameters &parameters);

    QSharedPointer<PluginOptions> pluginOptions() const;


    FileNamingParameters fileNamingParameters() const;
    void setFileNamingParameters(const FileNamingParameters &fnp);

private slots:
    void slotCurrentIdChanged (int id);

private:
    CustomWidgetWidgetsWizardPage *m_widgetsPage;
    CustomWidgetPluginWizardPage *m_pluginPage;
    int m_widgetPageId;
    int m_pluginPageId;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // CUSTOMWIDGETWIZARDDIALOG_H
