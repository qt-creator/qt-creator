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
    ~DetectCharRule() override {}

    void setChar(const QString &character);

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    DetectCharRule *doClone() const override { return new DetectCharRule(*this); }
    void doReplaceExpressions(const QStringList &captures) override;

    QChar m_char;
};

class Detect2CharsRule : public DynamicRule
{
public:
    ~Detect2CharsRule() override {}

    void setChar(const QString &character);
    void setChar1(const QString &character);

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    Detect2CharsRule *doClone() const override { return new Detect2CharsRule(*this); }
    void doReplaceExpressions(const QStringList &captures) override;

    QChar m_char;
    QChar m_char1;
};

class AnyCharRule : public Rule
{
public:
    ~AnyCharRule() override {}

    void setCharacterSet(const QString &s);

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    AnyCharRule *doClone() const override { return new AnyCharRule(*this); }

    QString m_characterSet;
};

class StringDetectRule : public DynamicRule
{
public:
    ~StringDetectRule() override {}

    void setString(const QString &s);
    void setInsensitive(const QString &insensitive);

protected:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    StringDetectRule *doClone() const override { return new StringDetectRule(*this); }
    void doReplaceExpressions(const QStringList &captures) override;

    QString m_string;
    int m_length = 0;
    Qt::CaseSensitivity m_caseSensitivity = Qt::CaseSensitive;
};

class WordDetectRule : public StringDetectRule
{
private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    WordDetectRule *doClone() const override { return new WordDetectRule(*this); }
};

class RegExprRule : public DynamicRule
{
public:
    ~RegExprRule() override;

    void setPattern(const QString &pattern);
    void setInsensitive(const QString &insensitive);
    void setMinimal(const QString &minimal);

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    RegExprRule *doClone() const override;
    void doReplaceExpressions(const QStringList &captures) override;
    void doProgressFinished() override;

    bool isExactMatch(ProgressData *progress);

    bool m_onlyBegin = false;
    bool m_isCached = false;
    int m_offset = 0;
    int m_length = 0;
    QStringList m_captures;
    QRegExp m_expression;
    ProgressData *m_progress = nullptr;
};

class KeywordRule : public Rule
{
public:
    KeywordRule(const QSharedPointer<HighlightDefinition> &definition);
    ~KeywordRule() override;

    void setInsensitive(const QString &insensitive);
    void setList(const QString &listName);

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    KeywordRule *doClone() const override { return new KeywordRule(*this); }

    bool m_overrideGlobal;
    Qt::CaseSensitivity m_localCaseSensitivity;
    QSharedPointer<KeywordList> m_list;
};

class IntRule : public Rule
{
public:
    ~IntRule() override {}

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    IntRule *doClone() const override { return new IntRule(*this); }
};

class FloatRule : public Rule
{
public:
    ~FloatRule() override {}

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    FloatRule *doClone() const override { return new FloatRule(*this); }
};

class HlCOctRule : public Rule
{
public:
    ~HlCOctRule() override {}

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    HlCOctRule *doClone() const override { return new HlCOctRule(*this); }
};

class HlCHexRule : public Rule
{
public:
    ~HlCHexRule() override {}

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    HlCHexRule *doClone() const override { return new HlCHexRule(*this); }
};

class HlCStringCharRule : public Rule
{
public:
    ~HlCStringCharRule() override {}

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    HlCStringCharRule *doClone() const override { return new HlCStringCharRule(*this); }
};

class HlCCharRule : public Rule
{
public:
    ~HlCCharRule() override {}

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    HlCCharRule *doClone() const override { return new HlCCharRule(*this); }
};

class RangeDetectRule : public Rule
{
public:
    ~RangeDetectRule() override {}

    void setChar(const QString &character);
    void setChar1(const QString &character);

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    RangeDetectRule *doClone() const override { return new RangeDetectRule(*this); }

    QChar m_char;
    QChar m_char1;
};

class LineContinueRule : public Rule
{
public:
    ~LineContinueRule() override {}

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    LineContinueRule *doClone() const override { return new LineContinueRule(*this); }
};

class DetectSpacesRule : public Rule
{
public:
    DetectSpacesRule();
    ~DetectSpacesRule() override {}

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    DetectSpacesRule *doClone() const override { return new DetectSpacesRule(*this); }
};

class DetectIdentifierRule : public Rule
{
public:
    ~DetectIdentifierRule() override {}

private:
    bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) override;
    DetectIdentifierRule *doClone() const override { return new DetectIdentifierRule(*this); }
};

} // namespace Internal
} // namespace TextEditor
