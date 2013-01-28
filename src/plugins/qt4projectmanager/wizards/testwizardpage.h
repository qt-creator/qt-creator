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

#ifndef TESTWIZARDPAGE_H
#define TESTWIZARDPAGE_H

#include <QWizardPage>

namespace Qt4ProjectManager {
namespace Internal {

namespace Ui {
    class TestWizardPage;
}

struct TestWizardParameters;

/* TestWizardPage: Let's the user input test class name, slot
 * (for which a CLassNameValidatingLineEdit is abused) and file name. */

class TestWizardPage : public QWizardPage {
    Q_OBJECT
public:
    explicit TestWizardPage(QWidget *parent = 0);
    ~TestWizardPage();
    virtual bool isComplete() const;

    TestWizardParameters parameters() const;
    QString sourcefileName() const;

public slots:
    void setProjectName(const QString &);

private slots:
    void slotClassNameEdited(const QString&);
    void slotFileNameEdited();
    void slotUpdateValid();

protected:
    void changeEvent(QEvent *e);

private:
    const QString m_sourceSuffix;
    const bool m_lowerCaseFileNames;
    Ui::TestWizardPage *ui;
    bool m_fileNameEdited;
    bool m_valid;

};

} // namespace Internal
} // namespace Qt4ProjectManager
#endif // TESTWIZARDPAGE_H
