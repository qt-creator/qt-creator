// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "speechactivity.h"

#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace VoiceInput::Internal {

const double kPauseSeconds = 0.6;

/*!
    \internal

    Returns whether the \a count \a samples, recorded at \a sampleRate, are
    loud enough to contain speech, and whether their end is quiet compared with
    their loudest part, which makes it independent of the microphone's gain.
    That tells silence from speech, but not noise from speech.
*/
SpeechActivity loudnessActivity(const float *samples, std::size_t count, int sampleRate)
{
    const float minimumPeak = 0.01f;
    const double quietShare = 0.15;
    SpeechActivity activity;
    float peak = 0;
    for (std::size_t i = 0; i < count; ++i)
        peak = std::max(peak, std::abs(samples[i]));
    activity.hasSpeech = peak >= minimumPeak;

    const auto window = std::size_t(kPauseSeconds * sampleRate);
    if (!activity.hasSpeech || count < 2 * window)
        return activity;
    const auto rms = [samples, window](std::size_t start) {
        double sum = 0;
        for (std::size_t i = start; i < start + window; ++i)
            sum += double(samples[i]) * samples[i];
        return std::sqrt(sum / double(window));
    };
    double loudest = 0;
    for (std::size_t start = 0; start + window <= count - window; start += window / 2)
        loudest = std::max(loudest, rms(start));
    activity.endsInPause = loudest > minimumPeak && rms(count - window) < quietShare * loudest;
    return activity;
}

/*!
    \class VoiceInput::Internal::LevelHistory
    \internal

    \brief The LevelHistory class keeps the levels of the latest \c capacity
    pieces of a recording.
*/

LevelHistory::LevelHistory(int capacity)
    : m_capacity(capacity)
{}

/*!
    Adds the \a level of the piece starting at \a firstSample of the recording.
*/
void LevelHistory::addLevel(qint64 firstSample, float level)
{
    m_bars.append({firstSample, level});
    if (m_bars.size() > m_capacity)
        m_bars.removeFirst();
}

void LevelHistory::clear()
{
    m_bars.clear();
}

/*!
    \internal

    Returns the height from 0 to 1 that shows the RMS \a level of audio, on a
    scale of decibels from silence to loud speech.
*/
float levelHeight(float level)
{
    const double minimumDb = -60;
    const double maximumDb = -12;
    const double db = 20 * std::log10(std::max(double(level), 1e-6));
    return float(std::clamp((db - minimumDb) / (maximumDb - minimumDb), 0.0, 1.0));
}

/*!
    \class VoiceInput::Internal::HighPassFilter
    \internal

    \brief The HighPassFilter class is a second-order Butterworth high-pass
    filter, for taking rumble out of a recording.
*/

HighPassFilter::HighPassFilter(double cutoffHz, int sampleRate)
{
    const double w0 = 2 * std::numbers::pi * cutoffHz / sampleRate;
    const double cosine = std::cos(w0);
    const double alpha = std::sin(w0) / std::numbers::sqrt2;
    const double a0 = 1 + alpha;
    m_b0 = (1 + cosine) / 2 / a0;
    m_b1 = -(1 + cosine) / a0;
    m_b2 = m_b0;
    m_a1 = -2 * cosine / a0;
    m_a2 = (1 - alpha) / a0;
}

float HighPassFilter::process(float sample)
{
    const double result = m_b0 * sample + m_b1 * m_x1 + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;
    m_x2 = m_x1;
    m_x1 = sample;
    m_y2 = m_y1;
    m_y1 = result;
    return float(result);
}

const int kKernelLobes = 16;
const int kKernelPhases = 256;

/*!
    \class VoiceInput::Internal::Resampler
    \internal

    Converts the sample rate with a windowed-sinc low-pass filter, so that
    what is above the lower Nyquist frequency does not fold back into the
    audio.
*/

Resampler::Resampler(int inputRate, int outputRate)
    : m_step(double(inputRate) / outputRate)
{
    // In cycles per input sample, a little below the lower Nyquist frequency.
    const double cutoff = 0.45 * std::min(inputRate, outputRate) / inputRate;
    m_halfWidth = int(std::ceil(kKernelLobes / (2 * cutoff)));
    m_table.resize(size_t(m_halfWidth) * kKernelPhases + 2);
    for (size_t index = 0; index < m_table.size(); ++index) {
        const double distance = double(index) / kKernelPhases;
        const double x = 2 * cutoff * distance;
        const double sinc = x == 0 ? 1 : std::sin(std::numbers::pi * x) / (std::numbers::pi * x);
        const double phase = std::min(distance / m_halfWidth, 1.0);
        const double blackman = 0.42 + 0.5 * std::cos(std::numbers::pi * phase)
                                + 0.08 * std::cos(2 * std::numbers::pi * phase);
        m_table[index] = float(sinc * blackman);
    }
    // The first output sample is the first input sample.
    m_input.assign(size_t(m_halfWidth), 0.0f);
    m_position = m_halfWidth;
}

float Resampler::kernel(double distance) const
{
    const double scaled = std::abs(distance) * kKernelPhases;
    const auto index = size_t(scaled);
    if (index + 1 >= m_table.size())
        return 0;
    const auto fraction = float(scaled - double(index));
    return m_table[index] * (1 - fraction) + m_table[index + 1] * fraction;
}

void Resampler::process(const float *samples, std::size_t count, std::vector<float> &output)
{
    if (m_step == 1) {
        output.insert(output.end(), samples, samples + count);
        return;
    }
    m_input.insert(m_input.end(), samples, samples + count);
    while (m_position + m_halfWidth < double(m_input.size())) {
        const auto center = std::ptrdiff_t(m_position);
        double sum = 0;
        double weights = 0;
        for (std::ptrdiff_t index = center - m_halfWidth + 1; index <= center + m_halfWidth;
             ++index) {
            const float weight = kernel(m_position - double(index));
            sum += weight * m_input[size_t(index)];
            weights += weight;
        }
        output.push_back(float(sum / weights));
        m_position += m_step;
    }
    const auto consumed = std::ptrdiff_t(m_position) - m_halfWidth;
    if (consumed > 0) {
        m_input.erase(m_input.begin(), m_input.begin() + consumed);
        m_position -= double(consumed);
    }
}

// 16-bit PCM mono, which every audio tool reads.
QByteArray wavFile(const std::vector<float> &samples, int sampleRate)
{
    const auto dataBytes = quint32(samples.size() * 2);
    QByteArray result;
    result.reserve(44 + qsizetype(dataBytes));
    const auto append16 = [&result](quint16 value) {
        const quint16 little = qToLittleEndian(value);
        result.append(reinterpret_cast<const char *>(&little), 2);
    };
    const auto append32 = [&result](quint32 value) {
        const quint32 little = qToLittleEndian(value);
        result.append(reinterpret_cast<const char *>(&little), 4);
    };
    result.append("RIFF");
    append32(36 + dataBytes);
    result.append("WAVEfmt ");
    append32(16);
    append16(1); // PCM
    append16(1); // channels
    append32(quint32(sampleRate));
    append32(quint32(sampleRate) * 2);
    append16(2); // bytes per frame
    append16(16);
    result.append("data");
    append32(dataBytes);
    for (const float sample : samples)
        append16(quint16(qint16(std::lround(std::clamp(sample, -1.0f, 1.0f) * 32767))));
    return result;
}

} // namespace VoiceInput::Internal
