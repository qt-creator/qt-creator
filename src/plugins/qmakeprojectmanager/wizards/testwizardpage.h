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

#include <QWizardPage>

namespace QmakeProjectManager {
namespace Internal {

namespace Ui { class TestWizardPage; }

struct TestWizardParameters;

/* TestWizardPage: Let's the user input test class name, slot
 * (for which a CLassNameValidatingLineEdit is abused) and file name. */

class TestWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit TestWizardPage(QWidget *parent = 0);
    ~TestWizardPage();
    virtual bool isComplete() const;

    TestWizardParameters parameters() const;
    QString sourcefileName() const;

    void setProjectName(const QString &);

private:
    void slotClassNameEdited(const QString&);
    void slotFileNameEdited();
    void slotUpdateValid();

    const QString m_sourceSuffix;
    const bool m_lowerCaseFileNames;
    Ui::TestWizardPage *ui;
    bool m_fileNameEdited;
    bool m_valid;
};

} // namespace Internal
} // namespace QmakeProjectManager
