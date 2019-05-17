/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "lsptypes.h"
#include "lsputils.h"

#include <utils/mimetypes/mimedatabase.h>
#include <utils/textutils.h>

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QMap>
#include <QTextBlock>
#include <QTextDocument>
#include <QVector>

namespace LanguageServerProtocol {

Utils::optional<DiagnosticSeverity> Diagnostic::severity() const
{
    if (auto val = optionalValue<int>(severityKey))
        return Utils::make_optional(static_cast<DiagnosticSeverity>(val.value()));
    return Utils::nullopt;
}

Utils::optional<Diagnostic::Code> Diagnostic::code() const
{
    QJsonValue codeValue = value(codeKey);
    auto it = find(codeKey);
    if (codeValue.isUndefined())
        return Utils::nullopt;
    QJsonValue::Type type = it.value().type();
    if (type != QJsonValue::String && type != QJsonValue::Double)
        return Utils::make_optional(Code(QString()));
    return Utils::make_optional(codeValue.isDouble() ? Code(codeValue.toInt())
                                                     : Code(codeValue.toString()));
}

void Diagnostic::setCode(const Diagnostic::Code &code)
{
    if (auto val = Utils::get_if<int>(&code))
        insert(codeKey, *val);
    else if (auto val = Utils::get_if<QString>(&code))
        insert(codeKey, *val);
}

bool Diagnostic::isValid(QStringList *error) const
{
    return check<Range>(error, rangeKey)
            && checkOptional<int>(error, severityKey)
            && (checkOptional<int>(error, codeKey) || checkOptional<QString>(error, codeKey))
            && checkOptional<QString>(error, sourceKey)
            && check<QString>(error, messageKey);
}

Utils::optional<WorkspaceEdit::Changes> WorkspaceEdit::changes() const
{
    auto it = find(changesKey);
    if (it == end())
        return Utils::nullopt;
    const QJsonObject &changesObject = it.value().toObject();
    Changes changesResult;
    for (const QString &key : changesObject.keys())
        changesResult[DocumentUri::fromProtocol(key)] = LanguageClientArray<TextEdit>(changesObject.value(key)).toList();
    return Utils::make_optional(changesResult);
}

void WorkspaceEdit::setChanges(const Changes &changes)
{
    QJsonObject changesObject;
    const auto end = changes.end();
    for (auto it = changes.begin(); it != end; ++it) {
        QJsonArray edits;
        for (const TextEdit &edit : it.value())
            edits.append(QJsonValue(edit));
        changesObject.insert(it.key().toFileName().toString(), edits);
    }
    insert(changesKey, changesObject);
}

WorkSpaceFolder::WorkSpaceFolder(const QString &uri, const QString &name)
{
    setUri(uri);
    setName(name);
}

MarkupOrString::MarkupOrString(const Utils::variant<QString, MarkupContent> &val)
    : Utils::variant<QString, MarkupContent>(val)
{ }

MarkupOrString::MarkupOrString(const QString &val)
    : Utils::variant<QString, MarkupContent>(val)
{ }

MarkupOrString::MarkupOrString(const MarkupContent &val)
    : Utils::variant<QString, MarkupContent>(val)
{ }

MarkupOrString::MarkupOrString(const QJsonValue &val)
{
    if (val.isString()) {
        emplace<QString>(val.toString());
    } else {
        MarkupContent markupContent(val.toObject());
        if (markupContent.isValid(nullptr))
            emplace<MarkupContent>(MarkupContent(val.toObject()));
    }
}

bool MarkupOrString::isValid(QStringList *error) const
{
    if (Utils::holds_alternative<MarkupContent>(*this) || Utils::holds_alternative<QString>(*this))
        return true;
    if (error) {
        *error << QCoreApplication::translate("LanguageServerProtocoll::MarkupOrString",
                                              "Expected a string or MarkupContent in MarkupOrString.");
    }
    return false;
}

QJsonValue MarkupOrString::toJson() const
{
    if (Utils::holds_alternative<QString>(*this))
        return Utils::get<QString>(*this);
    if (Utils::holds_alternative<MarkupContent>(*this))
        return QJsonValue(Utils::get<MarkupContent>(*this));
    return {};
}

bool SymbolInformation::isValid(QStringList *error) const
{
    return check<QString>(error, nameKey)
            && check<int>(error, kindKey)
            && check<Location>(error, locationKey)
            && checkOptional<QString>(error, containerNameKey);
}

bool TextDocumentEdit::isValid(QStringList *error) const
{
    return check<VersionedTextDocumentIdentifier>(error, idKey)
            && checkArray<TextEdit>(error, editsKey);
}

bool TextDocumentItem::isValid(QStringList *error) const
{
    return check<QString>(error, uriKey)
            && check<QString>(error, languageIdKey)
            && check<int>(error, versionKey)
            && check<QString>(error, textKey);
}

static QHash<Utils::MimeType, QString> mimeTypeLanguageIdMap()
{
    static QHash<Utils::MimeType, QString> hash;
    if (!hash.isEmpty())
        return hash;
    const QVector<QPair<QString, QString>> languageIdsForMimeTypeNames{
        {"text/x-python", "python"},
        {"text/x-bibtex", "bibtex"},
        {"application/vnd.coffeescript", "coffeescript"},
        {"text/x-chdr", "c"},
        {"text/x-csrc", "c"},
        {"text/x-c++hdr", "cpp"},
        {"text/x-c++src", "cpp"},
        {"text/x-moc", "cpp"},
        {"text/x-csharp", "csharp"},
        {"text/vnd.qtcreator.git.commit", "git-commit"},
        {"text/vnd.qtcreator.git.rebase", "git-rebase"},
        {"text/x-go", "go"},
        {"text/html", "html"},
        {"text/x-java", "java"},
        {"application/javascript", "javascript"},
        {"application/json", "json"},
        {"text/x-tex", "tex"},
        {"text/x-lua", "lua"},
        {"text/x-makefile", "makefile"},
        {"text/markdown", "markdown"},
        {"text/x-objcsrc", "objective-c"},
        {"text/x-objc++src", "objective-cpp"},
        {"application/x-perl", "perl"},
        {"application/x-php", "php"},
        {"application/x-ruby", "ruby"},
        {"text/rust", "rust"},
        {"text/x-sass", "sass"},
        {"text/x-scss", "scss"},
        {"application/x-shellscript", "shellscript"},
        {"application/xml", "xml"},
        {"application/xslt+xml", "xsl"},
        {"application/x-yaml", "yaml"},
    };
    for (const QPair<QString, QString> &languageIdForMimeTypeName : languageIdsForMimeTypeNames) {
        const Utils::MimeType &mimeType = Utils::mimeTypeForName(languageIdForMimeTypeName.first);
        if (mimeType.isValid()) {
            hash[mimeType] = languageIdForMimeTypeName.second;
            for (const QString &aliasName : mimeType.aliases()) {
                const Utils::MimeType &alias = Utils::mimeTypeForName(aliasName);
                if (alias.isValid())
                    hash[alias] = languageIdForMimeTypeName.second;
            }
        }
    }
    return hash;
}

QMap<QString, QString> languageIds()
{
    static const QMap<QString, QString> languages({
            {"Windows Bat","bat"                        },
            {"BibTeX","bibtex"                          },
            {"Clojure","clojure"                        },
            {"Coffeescript","coffeescript"              },
            {"C","c"                                    },
            {"C++","cpp"                                },
            {"C#","csharp"                              },
            {"CSS","css"                                },
            {"Diff","diff"                              },
            {"Dockerfile","dockerfile"                  },
            {"F#","fsharp"                              },
            {"Git commit","git-commit"                  },
            {"Git rebase","git-rebase"                  },
            {"Go","go"                                  },
            {"Groovy","groovy"                          },
            {"Handlebars","handlebars"                  },
            {"HTML","html"                              },
            {"Ini","ini"                                },
            {"Java","java"                              },
            {"JavaScript","javascript"                  },
            {"JSON","json"                              },
            {"LaTeX","latex"                            },
            {"Less","less"                              },
            {"Lua","lua"                                },
            {"Makefile","makefile"                      },
            {"Markdown","markdown"                      },
            {"Objective-C","objective-c"                },
            {"Objective-C++","objective-cpp"            },
            {"Perl6","perl6"                            },
            {"Perl","perl"                              },
            {"PHP","php"                                },
            {"Powershell","powershell"                  },
            {"Pug","jade"                               },
            {"Python","python"                          },
            {"R","r"                                    },
            {"Razor (cshtml)","razor"                   },
            {"Ruby","ruby"                              },
            {"Rust","rust"                              },
            {"Scss (syntax using curly brackets)","scss"},
            {"Sass (indented syntax)","sass"            },
            {"ShaderLab","shaderlab"                    },
            {"Shell Script (Bash)","shellscript"        },
            {"SQL","sql"                                },
            {"Swift","swift"                            },
            {"TypeScript","typescript"                  },
            {"TeX","tex"                                },
            {"Visual Basic","vb"                        },
            {"XML","xml"                                },
            {"XSL","xsl"                                },
            {"YAML","yaml"                              }
    });
    return languages;
}

QString TextDocumentItem::mimeTypeToLanguageId(const Utils::MimeType &mimeType)
{
    return mimeTypeLanguageIdMap().value(mimeType);
}

QString TextDocumentItem::mimeTypeToLanguageId(const QString &mimeTypeName)
{
    return mimeTypeToLanguageId(Utils::mimeTypeForName(mimeTypeName));
}

TextDocumentPositionParams::TextDocumentPositionParams()
    : TextDocumentPositionParams(TextDocumentIdentifier(), Position())
{

}

TextDocumentPositionParams::TextDocumentPositionParams(
        const TextDocumentIdentifier &document, const Position &position)
{
    setTextDocument(document);
    setPosition(position);
}

bool TextDocumentPositionParams::isValid(QStringList *error) const
{
    return check<TextDocumentIdentifier>(error, textDocumentKey)
            && check<Position>(error, positionKey);
}

Position::Position(int line, int character)
{
    setLine(line);
    setCharacter(character);
}

Position::Position(const QTextCursor &cursor)
    : Position(cursor.blockNumber(), cursor.positionInBlock())
{ }

int Position::toPositionInDocument(QTextDocument *doc) const
{
    const QTextBlock block = doc->findBlockByNumber(line());
    if (!block.isValid() || block.length() <= character())
        return -1;
    return block.position() + character();
}

QTextCursor Position::toTextCursor(QTextDocument *doc) const
{
    QTextCursor cursor(doc);
    cursor.setPosition(toPositionInDocument(doc));
    return cursor;
}

Range::Range(const Position &start, const Position &end)
{
    setStart(start);
    setEnd(end);
}

Range::Range(const QTextCursor &cursor)
{
    int line, character = 0;
    Utils::Text::convertPosition(cursor.document(), cursor.selectionStart(), &line, &character);
    if (line <= 0 || character <= 0)
        return;
    setStart(Position(line - 1, character - 1));
    Utils::Text::convertPosition(cursor.document(), cursor.selectionEnd(), &line, &character);
    if (line <= 0 || character <= 0)
        return;
    setEnd(Position(line - 1, character - 1));
}

bool Range::overlaps(const Range &range) const
{
    return contains(range.start()) || contains(range.end());
}

bool DocumentFilter::applies(const Utils::FileName &fileName, const Utils::MimeType &mimeType) const
{
    if (Utils::optional<QString> _scheme = scheme()) {
        if (_scheme.value() == fileName.toString())
            return true;
    }
    if (Utils::optional<QString> _pattern = pattern()) {
        QRegExp regexp(_pattern.value(),
                       Utils::HostOsInfo::fileNameCaseSensitivity(),
                       QRegExp::Wildcard);
        if (regexp.exactMatch(fileName.toString()))
            return true;
    }
    if (Utils::optional<QString> _lang = language()) {
        auto match = [&_lang](const Utils::MimeType &mimeType){
            return _lang.value() == TextDocumentItem::mimeTypeToLanguageId(mimeType);
        };
        if (mimeType.isValid() && match(mimeType))
            return true;
        return Utils::anyOf(Utils::mimeTypesForFileName(fileName.toString()), match);
    }
    // return false when any of the filter didn't match but return true when no filter was defined
    return !contains(schemeKey) && !contains(languageKey) && !contains(patternKey);
}

bool DocumentFilter::isValid(QStringList *error) const
{
    return Utils::allOf(QStringList{languageKey, schemeKey, patternKey}, [this, &error](auto key){
        return this->checkOptional<QString>(error, key);
    });
}

Utils::Link Location::toLink() const
{
    if (!isValid(nullptr))
        return Utils::Link();
    auto file = uri().toString(QUrl::FullyDecoded | QUrl::PreferLocalFile);
    return Utils::Link(file, range().start().line() + 1, range().start().character());
}

DocumentUri::DocumentUri(const QString &other)
    : QUrl(QUrl::fromPercentEncoding(other.toLocal8Bit()))
{ }

DocumentUri::DocumentUri(const Utils::FileName &other)
    : QUrl(QUrl::fromLocalFile(other.toString()))
{ }

Utils::FileName DocumentUri::toFileName() const
{
    return isLocalFile() ? Utils::FileName::fromUserInput(QUrl(*this).toLocalFile())
                         : Utils::FileName();
}

} // namespace LanguageServerProtocol
