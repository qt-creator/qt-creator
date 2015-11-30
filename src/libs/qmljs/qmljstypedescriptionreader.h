/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSTYPEDESCRIPTIONREADER_H
#define QMLJSTYPEDESCRIPTIONREADER_H

#include "qmljs_global.h"
#include <qmljs/parser/qmljsastfwd_p.h>

#include <languageutils/fakemetaobject.h>

// for Q_DECLARE_TR_FUNCTIONS
#include <QCoreApplication>

QT_BEGIN_NAMESPACE
class QIODevice;
class QBuffer;
QT_END_NAMESPACE

namespace QmlJS {

class ModuleApiInfo;
namespace AST {
class UiProgram;
class UiObjectDefinition;
class UiScriptBinding;
class SourceLocation;
}

class QMLJS_EXPORT TypeDescriptionReader
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::TypeDescriptionReader)

public:
    explicit TypeDescriptionReader(const QString &fileName, const QString &data);
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

    QString _fileName;
    QString _source;
    QString _errorMessage;
    QString _warningMessage;
    QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> *_objects;
    QList<ModuleApiInfo> *_moduleApis;
};

} // namespace QmlJS

#endif // QMLJSTYPEDESCRIPTIONREADER_H
