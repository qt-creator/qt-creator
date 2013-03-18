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

#ifndef PYTHON_SOURCEGENERATOR_H
#define PYTHON_SOURCEGENERATOR_H

#include "../pythoneditor_global.h"
#include <utils/newclasswidget.h>
#include <QSet>
#include <QString>

namespace PythonEditor {

class PYEDITOR_EXPORT SourceGenerator
{
public:
    enum QtBinding {
        PySide,
        PyQt
    };

    enum QtVersion {
        Qt4,
        Qt5
    };

    SourceGenerator();
    ~SourceGenerator();

    void setPythonQtBinding(QtBinding binding);
    void setPythonQtVersion(QtVersion version);

    QString generateClass(const QString &className,
                          const QString &baseClass,
                          Utils::NewClassWidget::ClassType classType) const;

    QString generateQtMain(const QString &windowTitle) const;

private:
    QString qtModulesImport(const QSet<QString> &modules) const;
    QString moduleForQWidget() const;

    QtBinding m_pythonQtBinding;
    QtVersion m_pythonQtVersion;
};

} // namespace PythonEditor

#endif // PYTHON_SOURCEGENERATOR_H
