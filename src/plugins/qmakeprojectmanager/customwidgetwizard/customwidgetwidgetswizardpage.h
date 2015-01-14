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

#ifndef CUSTOMWIDGETWIDGETSWIZARDPAGE_H
#define CUSTOMWIDGETWIDGETSWIZARDPAGE_H

#include "pluginoptions.h"
#include "filenamingparameters.h"

#include <QList>
#include <QWizardPage>

QT_BEGIN_NAMESPACE
class QStackedLayout;
QT_END_NAMESPACE

namespace QmakeProjectManager {
namespace Internal {

class ClassDefinition;
struct PluginOptions;

namespace Ui { class CustomWidgetWidgetsWizardPage; }

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
