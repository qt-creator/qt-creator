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

#ifndef QMLJSTYPEDESCRIPTIONREADER_H
#define QMLJSTYPEDESCRIPTIONREADER_H

#include <languageutils/fakemetaobject.h>
#include <languageutils/componentversion.h>
#include "qmljsdocument.h"

// for Q_DECLARE_TR_FUNCTIONS
#include <QCoreApplication>

QT_BEGIN_NAMESPACE
class QIODevice;
class QBuffer;
QT_END_NAMESPACE

namespace QmlJS {

namespace AST {
class UiProgram;
class UiObjectDefinition;
class UiScriptBinding;
class SourceLocation;
}

class TypeDescriptionReader
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::TypeDescriptionReader)

public:
    explicit TypeDescriptionReader(const QString &data);
    ~TypeDescriptionReader();

    bool operator()(
            QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> *objects,
            QList<ModuleApiInfo> *moduleApis);
    QString errorMessage() const;
    QString warningMessage() const;

private:
    void readDocument(AST::UiProgram *ast);
    void readModule(AST::UiObjectDefinition *ast);
    void readComponent(AST::UiObjectDefinition *ast);
    void readModuleApi(AST::UiObjectDefinition *ast);
    void readSignalOrMethod(AST::UiObjectDefinition *ast, bool isMethod, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readProperty(AST::UiObjectDefinition *ast, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readEnum(AST::UiObjectDefinition *ast, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readParameter(AST::UiObjectDefinition *ast, LanguageUtils::FakeMetaMethod *fmm);

    QString readStringBinding(AST::UiScriptBinding *ast);
    bool readBoolBinding(AST::UiScriptBinding *ast);
    double readNumericBinding(AST::UiScriptBinding *ast);
    LanguageUtils::ComponentVersion readNumericVersionBinding(AST::UiScriptBinding *ast);
    int readIntBinding(AST::UiScriptBinding *ast);
    void readExports(AST::UiScriptBinding *ast, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readMetaObjectRevisions(AST::UiScriptBinding *ast, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readEnumValues(AST::UiScriptBinding *ast, LanguageUtils::FakeMetaEnum *fme);

    void addError(const AST::SourceLocation &loc, const QString &message);
    void addWarning(const AST::SourceLocation &loc, const QString &message);

    QString _source;
    QString _errorMessage;
    QString _warningMessage;
    QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> *_objects;
    QList<ModuleApiInfo> *_moduleApis;
};

} // namespace QmlJS

#endif // QMLJSTYPEDESCRIPTIONREADER_H
