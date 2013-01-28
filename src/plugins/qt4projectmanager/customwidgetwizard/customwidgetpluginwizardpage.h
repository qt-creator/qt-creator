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

#ifndef CUSTOMWIDGETPLUGINWIZARDPAGE_H
#define CUSTOMWIDGETPLUGINWIZARDPAGE_H

#include "filenamingparameters.h"

#include <QWizardPage>
#include <QSharedPointer>

namespace Qt4ProjectManager {
namespace Internal {

struct PluginOptions;
class CustomWidgetWidgetsWizardPage;

namespace Ui {
    class CustomWidgetPluginWizardPage;
}

class CustomWidgetPluginWizardPage : public QWizardPage {
    Q_OBJECT
public:
    explicit CustomWidgetPluginWizardPage(QWidget *parent = 0);
    virtual ~CustomWidgetPluginWizardPage();

    void init(const CustomWidgetWidgetsWizardPage *widgetsPage);

    virtual bool isComplete() const;

    FileNamingParameters fileNamingParameters() const { return m_fileNamingParameters; }
    void setFileNamingParameters(const FileNamingParameters &fnp) {m_fileNamingParameters = fnp; }

    // Fills the plugin fields, excluding widget list.
    QSharedPointer<PluginOptions> basicPluginOptions() const;

protected:
    void changeEvent(QEvent *e);

private slots:
    void on_collectionClassEdit_textChanged();
    void on_collectionHeaderEdit_textChanged();
    void slotCheckCompleteness();

private:
    inline QString collectionClassName() const;
    inline QString pluginName() const;
    void setCollectionEnabled(bool enColl);

    Ui::CustomWidgetPluginWizardPage *m_ui;
    FileNamingParameters m_fileNamingParameters;
    int m_classCount;
    bool m_complete;
};

} // namespace Internal
} // namespace Qt4ProjectManager
#endif // CUSTOMWIDGETPLUGINWIZARDPAGE_H
