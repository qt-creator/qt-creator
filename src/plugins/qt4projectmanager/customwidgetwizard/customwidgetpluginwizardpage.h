/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CUSTOMWIDGETPLUGINWIZARDPAGE_H
#define CUSTOMWIDGETPLUGINWIZARDPAGE_H

#include "filenamingparameters.h"

#include <QtGui/QWizardPage>
#include <QtCore/QSharedPointer>

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
    CustomWidgetPluginWizardPage(QWidget *parent = 0);
    ~CustomWidgetPluginWizardPage();

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
