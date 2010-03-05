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

#ifndef CUSTOMWIDGETWIDGETSWIZARDPAGE_H
#define CUSTOMWIDGETWIDGETSWIZARDPAGE_H

#include "pluginoptions.h"
#include "filenamingparameters.h"

#include <QtCore/QList>
#include <QtGui/QWizardPage>

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

private Q_SLOTS:
    void on_classList_classAdded(const QString &name);
    void on_classList_classDeleted(int index);
    void on_classList_classRenamed(int index, const QString &newName);
    void slotCheckCompleteness();

private:
    void updatePluginTab();

    Ui::CustomWidgetWidgetsWizardPage *m_ui;
    QList<ClassDefinition *> m_uiClassDefs;
    QStackedLayout *m_tabStack;
    QWidget *m_dummyTab;
    FileNamingParameters m_fileNamingParameters;
    bool m_complete;
};

}
}

#endif // CUSTOMWIDGETWIDGETSWIZARDPAGE_H
