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
