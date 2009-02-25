/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CPPCLASSWIZARD_H
#define CPPCLASSWIZARD_H

#include <coreplugin/basefilewizard.h>

#include <QtCore/QStringList>
#include <QtGui/QWizardPage>
#include <QtGui/QWizard>

namespace Core {
namespace Utils {

class NewClassWidget;

} // namespace Utils
} // namespace Core

namespace CppEditor {
namespace Internal {

class ClassNamePage : public QWizardPage
{
    Q_OBJECT

public:
    ClassNamePage(const QString &sourceSuffix,
                  const QString &headerSuffix,
                  QWidget *parent = 0);

    bool isComplete() const { return m_isValid; }
    Core::Utils::NewClassWidget *newClassWidget() const { return m_newClassWidget; }

private slots:
    void slotValidChanged();

private:
    Core::Utils::NewClassWidget *m_newClassWidget;
    bool m_isValid;
};


struct CppClassWizardParameters
{
    QString className;
    QString headerFile;
    QString sourceFile;
    QString baseClass;
    QString path;
};

class CppClassWizardDialog : public QWizard
{
    Q_OBJECT
    Q_DISABLE_COPY(CppClassWizardDialog)
public:
    explicit CppClassWizardDialog(const QString &sourceSuffix,
                                  const QString &headerSuffix,
                                  QWidget *parent = 0);

    void setPath(const QString &path);
    CppClassWizardParameters parameters() const;

private:
    ClassNamePage *m_classNamePage;
};


class CppClassWizard : public Core::BaseFileWizard
{
    Q_OBJECT
public:
    explicit CppClassWizard(const Core::BaseFileWizardParameters &parameters,
                            QObject *parent = 0);

protected:
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;


    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;
    QString sourceSuffix() const;
    QString headerSuffix() const;

private:
    static bool generateHeaderAndSource(const CppClassWizardParameters &params,
                                        QString *header, QString *source);

};

} // namespace Internal
} // namespace CppEditor

#endif // CPPCLASSWIZARD_H
