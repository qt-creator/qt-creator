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

#ifndef QMLPROJECTIMPORTWIZARD_H
#define QMLPROJECTIMPORTWIZARD_H

#include <coreplugin/basefilewizard.h>

#include <QtGui/QWizard>

QT_BEGIN_NAMESPACE
class QDir;
class QFileInfo;
class QModelIndex;
class QStringList;
QT_END_NAMESPACE

namespace Utils {
class FileWizardPage;
} // namespace Utils

namespace QmlProjectManager {
namespace Internal {

class QmlProjectImportWizardDialog : public QWizard
{
    Q_OBJECT

public:
    QmlProjectImportWizardDialog(QWidget *parent = 0);
    virtual ~QmlProjectImportWizardDialog();

    QString path() const;
    void setPath(const QString &path);

    QString projectName() const;

private:
    Utils::FileWizardPage *m_firstPage;
};

class QmlProjectImportWizard : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    QmlProjectImportWizard();
    virtual ~QmlProjectImportWizard();

    static Core::BaseFileWizardParameters parameters();

protected:
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;

    virtual bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);
};

} // end of namespace Internal
} // end of namespace QmlProjectManager

#endif // QMLPROJECTIMPORTWIZARD_H
