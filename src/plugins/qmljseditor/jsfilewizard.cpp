/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "jsfilewizard.h"

#include <qmljstools/qmljstoolsconstants.h>

#include <coreplugin/basefilewizard.h>

#include <utils/filewizardpage.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QTextStream>
#include <QWizard>
#include <QPushButton>
#include <QBoxLayout>
#include <QCheckBox>

namespace {
class JsFileOptionsPage : public QWizardPage
{
    Q_OBJECT
public:
    JsFileOptionsPage()
    {
        setTitle(tr("Options"));

        QVBoxLayout *layout = new QVBoxLayout;
        m_library = new QCheckBox(tr("JavaScript library"));
        m_library->setToolTip(
                    tr("Usually each QML component instance has a unique copy of\n"
                       "imported JavaScript libraries. Indicating that a JavaScript file is\n"
                       "a library means that a single instance will be shared among\n"
                       "all components. JavaScript libraries will not be able to access\n"
                       "QML component instance objects and properties directly."));
        layout->addWidget(m_library);
        setLayout(layout);
    }

    bool isLibrary() const
    {
        return m_library->isChecked();
    }

private:
    QCheckBox *m_library;
};

class JsFileWizardDialog : public Core::BaseFileWizard
{
    Q_OBJECT
public:
    JsFileWizardDialog(QWidget *parent = 0) :
        Core::BaseFileWizard(parent)
    {
        addPage(new Utils::FileWizardPage);
        addPage(new JsFileOptionsPage);
    }
};
} // anonymous namespace

using namespace QmlJSEditor;

JsFileWizard::JsFileWizard()
{
}

Core::GeneratedFiles JsFileWizard::generateFiles(const QWizard *w,
                                                 QString * /*errorMessage*/) const
{
    const Core::BaseFileWizard *wizard = qobject_cast<const Core::BaseFileWizard *>(w);
    Utils::FileWizardPage *filePage = wizard->find<Utils::FileWizardPage>();
    QTC_ASSERT(filePage, return Core::GeneratedFiles());
    JsFileOptionsPage *optionPage = wizard->find<JsFileOptionsPage>();
    QTC_ASSERT(optionPage, return Core::GeneratedFiles());


    const QString path = filePage->path();
    const QString name = filePage->fileName();

    const QString mimeType = QLatin1String(QmlJSTools::Constants::JS_MIMETYPE);
    const QString fileName = Core::BaseFileWizardFactory::buildFileName(path, name, preferredSuffix(mimeType));

    Core::GeneratedFile file(fileName);
    file.setContents(fileContents(fileName, optionPage->isLibrary()));
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    return Core::GeneratedFiles() << file;
}

QString JsFileWizard::fileContents(const QString &, bool isLibrary) const
{
    QString contents;
    QTextStream str(&contents);

    if (isLibrary)
        str << QLatin1String(".pragma library\n\n");
    str << QLatin1String("function func() {\n")
        << QLatin1String("\n")
        << QLatin1String("}\n");

    return contents;
}

Core::BaseFileWizard *JsFileWizard::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    JsFileWizardDialog *wizard = new JsFileWizardDialog(parent);
    wizard->setWindowTitle(tr("New %1").arg(displayName()));

    Utils::FileWizardPage *page = wizard->find<Utils::FileWizardPage>();
    page->setPath(parameters.defaultPath());

    foreach (QWizardPage *p, parameters.extensionPages())
        wizard->addPage(p);
    return wizard;
}

#include "jsfilewizard.moc"
