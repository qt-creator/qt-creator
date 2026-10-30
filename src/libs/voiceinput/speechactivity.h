// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

#include <cstddef>
#include <vector>

namespace VoiceInput::Internal {

struct SpeechActivity
{
    bool hasSpeech = false;
    bool endsInPause = false;
};

SpeechActivity loudnessActivity(const float *samples, std::size_t count, int sampleRate);

class LevelHistory
{
public:
    struct Bar
    {
        qint64 firstSample = 0;
        float level = 0;
    };

    explicit LevelHistory(int capacity);

    void addLevel(qint64 firstSample, float level);
    void clear();

    const QList<Bar> &bars() const { return m_bars; }

private:
    int m_capacity;
    QList<Bar> m_bars;
};

float levelHeight(float level);

class HighPassFilter
{
public:
    HighPassFilter(double cutoffHz, int sampleRate);

    float process(float sample);

private:
    double m_b0;
    double m_b1;
    double m_b2;
    double m_a1;
    double m_a2;
    double m_x1 = 0;
    double m_x2 = 0;
    double m_y1 = 0;
    double m_y2 = 0;
};

class Resampler
{
public:
    Resampler(int inputRate, int outputRate);

    void process(const float *samples, std::size_t count, std::vector<float> &output);

private:
    float kernel(double distance) const;

    double m_step;
    int m_halfWidth;
    std::vector<float> m_table;
    std::vector<float> m_input;
    double m_position;
};

QByteArray wavFile(const std::vector<float> &samples, int sampleRate);

} // namespace VoiceInput::Internal
