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

#ifndef CUSTOMWIDGETWIDGETSWIZARDPAGE_H
#define CUSTOMWIDGETWIDGETSWIZARDPAGE_H

#include "pluginoptions.h"
#include "filenamingparameters.h"

#include <QList>
#include <QWizardPage>

QT_BEGIN_NAMESPACE
class QStackedLayout;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class ClassDefinition;
struct PluginOptions;

namespace Ui {
    class CustomWidgetWidgetsWizardPage;
}

class CustomWidgetWidgetsWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit CustomWidgetWidgetsWizardPage(QWidget *parent = 0);
    virtual ~CustomWidgetWidgetsWizardPage();

    QList<PluginOptions::WidgetOptions> widgetOptions() const;

    virtual bool isComplete() const;

    FileNamingParameters fileNamingParameters() const { return m_fileNamingParameters; }
    void setFileNamingParameters(const FileNamingParameters &fnp) {m_fileNamingParameters = fnp; }

    int classCount() const { return m_uiClassDefs.size(); }
    QString classNameAt(int i) const;

    virtual void initializePage();

private Q_SLOTS:
    void on_classList_classAdded(const QString &name);
    void on_classList_classDeleted(int index);
    void on_classList_classRenamed(int index, const QString &newName);
    void slotCheckCompleteness();
    void slotCurrentRowChanged(int);

private:
    void updatePluginTab();

    Ui::CustomWidgetWidgetsWizardPage *m_ui;
    QList<ClassDefinition *> m_uiClassDefs;
    QStackedLayout *m_tabStackLayout;
    FileNamingParameters m_fileNamingParameters;
    bool m_complete;
};

}
}

#endif // CUSTOMWIDGETWIDGETSWIZARDPAGE_H
