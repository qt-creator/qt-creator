/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLJSEDITORFACTORY_H
#define QMLJSEDITORFACTORY_H

#include <coreplugin/editormanager/ieditorfactory.h>

#include <QtCore/QStringList>

namespace TextEditor {
class TextEditorActionHandler;
}

namespace QmlJSEditor {
namespace Internal {

class QmlJSEditorActionHandler;

class QmlJSEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    QmlJSEditorFactory(QObject *parent);
    ~QmlJSEditorFactory();

    virtual QStringList mimeTypes() const;
    // IEditorFactory
    QString id() const;
    QString displayName() const;
    Core::IFile *open(const QString &fileName);
    Core::IEditor *createEditor(QWidget *parent);

private slots:
    void activateQmlDesigner();
    void neverAskAgainAboutQmlDesigner();
    void updateEditorInfoBar(Core::IEditor *editor);

private:
    QStringList m_mimeTypes;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSEDITORFACTORY_H
