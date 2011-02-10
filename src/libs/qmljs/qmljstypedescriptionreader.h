#ifndef QMLJSTYPEDESCRIPTIONREADER_H
#define QMLJSTYPEDESCRIPTIONREADER_H

#include <languageutils/fakemetaobject.h>

#include <QtCore/QScopedPointer>

// for Q_DECLARE_TR_FUNCTIONS
#include <QtCore/QCoreApplication>

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

    bool operator()(QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> *objects);
    QString errorMessage() const;

private:
    void readDocument(AST::UiProgram *ast);
    void readModule(AST::UiObjectDefinition *ast);
    void readComponent(AST::UiObjectDefinition *ast);
    void readSignalOrMethod(AST::UiObjectDefinition *ast, bool isMethod, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readProperty(AST::UiObjectDefinition *ast, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readEnum(AST::UiObjectDefinition *ast, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readParameter(AST::UiObjectDefinition *ast, LanguageUtils::FakeMetaMethod *fmm);

    QString readStringBinding(AST::UiScriptBinding *ast);
    bool readBoolBinding(AST::UiScriptBinding *ast);
    void readExports(AST::UiScriptBinding *ast, LanguageUtils::FakeMetaObject::Ptr fmo);
    void readEnumValues(AST::UiScriptBinding *ast, LanguageUtils::FakeMetaEnum *fme);
    void addError(const AST::SourceLocation &loc, const QString &message);

    QString _source;
    QString _errorMessage;
    QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> *_objects;
};

} // namespace QmlJS

#endif // QMLJSTYPEDESCRIPTIONREADER_H
