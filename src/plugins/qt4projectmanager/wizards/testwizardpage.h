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

#ifndef TESTWIZARDPAGE_H
#define TESTWIZARDPAGE_H

#include <QtGui/QWizardPage>

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
