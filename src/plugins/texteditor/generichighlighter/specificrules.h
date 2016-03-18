/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "rule.h"
#include "dynamicrule.h"

#include <QChar>
#include <QStringList>
#include <QRegExp>
#include <QSharedPointer>

namespace TextEditor {
namespace Internal {

class KeywordList;
class HighlightDefinition;

class DetectCharRule : public DynamicRule
{
public:
    virtual ~DetectCharRule() {}

    void setChar(const QString &character);

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual DetectCharRule *doClone() const { return new DetectCharRule(*this); }
    virtual void doReplaceExpressions(const QStringList &captures);

    QChar m_char;
};

class Detect2CharsRule : public DynamicRule
{
public:
    virtual ~Detect2CharsRule() {}

    void setChar(const QString &character);
    void setChar1(const QString &character);

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual Detect2CharsRule *doClone() const { return new Detect2CharsRule(*this); }
    virtual void doReplaceExpressions(const QStringList &captures);

    QChar m_char;
    QChar m_char1;
};

class AnyCharRule : public Rule
{
public:
    virtual ~AnyCharRule() {}

    void setCharacterSet(const QString &s);

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual AnyCharRule *doClone() const { return new AnyCharRule(*this); }

    QString m_characterSet;
};

class StringDetectRule : public DynamicRule
{
public:
    virtual ~StringDetectRule() {}

    void setString(const QString &s);
    void setInsensitive(const QString &insensitive);

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual StringDetectRule *doClone() const { return new StringDetectRule(*this); }
    virtual void doReplaceExpressions(const QStringList &captures);

    QString m_string;
    int m_length;
    Qt::CaseSensitivity m_caseSensitivity;
};

class RegExprRule : public DynamicRule
{
public:
    RegExprRule() : m_onlyBegin(false), m_isCached(false) {}
    virtual ~RegExprRule() {}

    void setPattern(const QString &pattern);
    void setInsensitive(const QString &insensitive);
    void setMinimal(const QString &minimal);

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual RegExprRule *doClone() const { return new RegExprRule(*this); }
    virtual void doReplaceExpressions(const QStringList &captures);
    virtual void doProgressFinished();

    bool isExactMatch(ProgressData *progress);

    bool m_onlyBegin;
    bool m_isCached;
    int m_offset;
    int m_length;
    QStringList m_captures;
    QRegExp m_expression;
};

class KeywordRule : public Rule
{
public:
    KeywordRule(const QSharedPointer<HighlightDefinition> &definition);
    virtual ~KeywordRule();

    void setInsensitive(const QString &insensitive);
    void setList(const QString &listName);

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual KeywordRule *doClone() const { return new KeywordRule(*this); }

    bool m_overrideGlobal;
    Qt::CaseSensitivity m_localCaseSensitivity;
    QSharedPointer<KeywordList> m_list;
};

class IntRule : public Rule
{
public:
    virtual ~IntRule() {}

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual IntRule *doClone() const { return new IntRule(*this); }
};

class FloatRule : public Rule
{
public:
    virtual ~FloatRule() {}

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual FloatRule *doClone() const { return new FloatRule(*this); }
};

class HlCOctRule : public Rule
{
public:
    virtual ~HlCOctRule() {}

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual HlCOctRule *doClone() const { return new HlCOctRule(*this); }
};

class HlCHexRule : public Rule
{
public:
    virtual ~HlCHexRule() {}

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual HlCHexRule *doClone() const { return new HlCHexRule(*this); }
};

class HlCStringCharRule : public Rule
{
public:
    virtual ~HlCStringCharRule() {}

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual HlCStringCharRule *doClone() const { return new HlCStringCharRule(*this); }
};

class HlCCharRule : public Rule
{
public:
    virtual ~HlCCharRule() {}

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual HlCCharRule *doClone() const { return new HlCCharRule(*this); }
};

class RangeDetectRule : public Rule
{
public:
    virtual ~RangeDetectRule() {}

    void setChar(const QString &character);
    void setChar1(const QString &character);

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual RangeDetectRule *doClone() const { return new RangeDetectRule(*this); }

    QChar m_char;
    QChar m_char1;
};

class LineContinueRule : public Rule
{
public:
    virtual ~LineContinueRule() {}

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual LineContinueRule *doClone() const { return new LineContinueRule(*this); }
};

class DetectSpacesRule : public Rule
{
public:
    DetectSpacesRule();
    virtual ~DetectSpacesRule() {}

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual DetectSpacesRule *doClone() const { return new DetectSpacesRule(*this); }
};

class DetectIdentifierRule : public Rule
{
public:
    virtual ~DetectIdentifierRule() {}

private:
    virtual bool doMatchSucceed(const QString &text,
                                const int length,
                                ProgressData *progress);
    virtual DetectIdentifierRule *doClone() const { return new DetectIdentifierRule(*this); }
};

} // namespace Internal
} // namespace TextEditor
