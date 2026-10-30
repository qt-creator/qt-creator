// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "speechactivity.h"

#include <QTest>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

using namespace VoiceInput::Internal;

const int kSampleRate = 16000;
static void appendTone(std::vector<float> &samples, double seconds, float amplitude)
{
    const auto count = std::size_t(seconds * kSampleRate);
    for (std::size_t i = 0; i < count; ++i) {
        samples.push_back(
            amplitude * float(std::sin(2 * std::numbers::pi * 220 * double(i) / kSampleRate)));
    }
}

// The amplitude of a tone of frequencyHz after the filter has settled.
static double filteredAmplitude(double frequencyHz)
{
    HighPassFilter filter(100, kSampleRate);
    double peak = 0;
    for (int i = 0; i < kSampleRate; ++i) {
        const float in = float(std::sin(2 * std::numbers::pi * frequencyHz * i / kSampleRate));
        const float out = filter.process(in);
        if (i > kSampleRate / 2)
            peak = std::max(peak, double(std::abs(out)));
    }
    return peak;
}

static std::vector<float> sine(double frequencyHz, int sampleRate, double seconds)
{
    std::vector<float> result(std::size_t(seconds * sampleRate));
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = float(0.5 * std::sin(2 * std::numbers::pi * frequencyHz * double(i) / sampleRate));
    return result;
}

static std::vector<float> resampled(const std::vector<float> &input, int inputRate)
{
    Resampler resampler(inputRate, kSampleRate);
    std::vector<float> output;
    resampler.process(input.data(), input.size(), output);
    return output;
}

// Leaves out the first and last tenth of a second, where the filter settles.
static float settledPeak(const std::vector<float> &samples)
{
    float peak = 0;
    for (std::size_t i = kSampleRate / 10; i + kSampleRate / 10 < samples.size(); ++i)
        peak = std::max(peak, std::abs(samples[i]));
    return peak;
}

class tst_VoiceInput : public QObject
{
    Q_OBJECT

private slots:
    void loudnessActivity_data();
    void loudnessActivity();
    void highPassFilterRemovesRumble();
    void levelHistoryKeepsTheLatestLevels();
    void levelHeight();
    void resamplerKeepsSpeech_data();
    void resamplerKeepsSpeech();
    void resamplerRemovesWhatCannotBeKept();
    void resamplerWorksInChunks();
    void wavFile();
};

void tst_VoiceInput::loudnessActivity_data()
{
    QTest::addColumn<std::vector<float>>("samples");
    QTest::addColumn<bool>("hasSpeech");
    QTest::addColumn<bool>("endsInPause");

    std::vector<float> speechThenSilence;
    appendTone(speechThenSilence, 2, 0.3f);
    appendTone(speechThenSilence, 0.7, 0.f);
    QTest::newRow("speech, then silence") << speechThenSilence << true << true;

    std::vector<float> quietMicrophone;
    appendTone(quietMicrophone, 2, 0.03f);
    appendTone(quietMicrophone, 0.7, 0.002f);
    QTest::newRow("quiet microphone, then near silence") << quietMicrophone << true << true;

    std::vector<float> speaking;
    appendTone(speaking, 3, 0.3f);
    QTest::newRow("still speaking") << speaking << true << false;

    std::vector<float> shortPause;
    appendTone(shortPause, 2, 0.3f);
    appendTone(shortPause, 0.3, 0.f);
    QTest::newRow("pause shorter than a window") << shortPause << true << false;

    std::vector<float> silence;
    appendTone(silence, 3, 0.005f);
    QTest::newRow("near silence") << silence << false << false;

    std::vector<float> tooShort;
    appendTone(tooShort, 0.5, 0.3f);
    appendTone(tooShort, 0.5, 0.f);
    QTest::newRow("too short to tell") << tooShort << true << false;
}

void tst_VoiceInput::loudnessActivity()
{
    QFETCH(std::vector<float>, samples);
    QFETCH(bool, hasSpeech);
    QFETCH(bool, endsInPause);
    const SpeechActivity activity
        = VoiceInput::Internal::loudnessActivity(samples.data(), samples.size(), kSampleRate);
    QCOMPARE(activity.hasSpeech, hasSpeech);
    QCOMPARE(activity.endsInPause, endsInPause);
}

void tst_VoiceInput::highPassFilterRemovesRumble()
{
    QVERIFY2(filteredAmplitude(30) < 0.15, qPrintable(QString::number(filteredAmplitude(30))));
    QVERIFY2(filteredAmplitude(100) > 0.6 && filteredAmplitude(100) < 0.8,
             qPrintable(QString::number(filteredAmplitude(100))));
    QVERIFY2(filteredAmplitude(1000) > 0.98, qPrintable(QString::number(filteredAmplitude(1000))));
}

void tst_VoiceInput::levelHistoryKeepsTheLatestLevels()
{
    LevelHistory history(3);
    for (int i = 0; i < 5; ++i)
        history.addLevel(i * 1600, float(i));
    QCOMPARE(history.bars().size(), 3);
    QCOMPARE(history.bars().first().firstSample, 2 * 1600);
    QCOMPARE(history.bars().last().level, 4.f);
    history.clear();
    QVERIFY(history.bars().isEmpty());
}

void tst_VoiceInput::levelHeight()
{
    QCOMPARE(VoiceInput::Internal::levelHeight(0), 0.f);
    QCOMPARE(VoiceInput::Internal::levelHeight(0.001f), 0.f);  // -60 dB
    QCOMPARE(VoiceInput::Internal::levelHeight(1.f), 1.f);
    const float quiet = VoiceInput::Internal::levelHeight(0.01f); // -40 dB
    const float loud = VoiceInput::Internal::levelHeight(0.1f);   // -20 dB
    QVERIFY(quiet > 0.3f && quiet < 0.5f);
    QVERIFY(loud > 0.7f && loud < 0.9f);
}

void tst_VoiceInput::resamplerKeepsSpeech_data()
{
    QTest::addColumn<int>("inputRate");

    QTest::newRow("48 kHz") << 48000;
    QTest::newRow("44.1 kHz") << 44100;
    QTest::newRow("16 kHz") << 16000;
    QTest::newRow("8 kHz") << 8000;
}

void tst_VoiceInput::resamplerKeepsSpeech()
{
    QFETCH(int, inputRate);
    const std::vector<float> output = resampled(sine(1000, inputRate, 1), inputRate);
    const std::vector<float> expected = sine(1000, kSampleRate, 1);
    QVERIFY2(std::abs(qint64(output.size()) - kSampleRate) < 100,
             qPrintable(QString::number(output.size())));
    float error = 0;
    for (std::size_t i = kSampleRate / 10; i + kSampleRate / 10 < output.size(); ++i)
        error = std::max(error, std::abs(output[i] - expected[i]));
    QVERIFY2(error < 0.01, qPrintable(QString::number(error)));
}

void tst_VoiceInput::resamplerRemovesWhatCannotBeKept()
{
    // 10 kHz is above the 8 kHz that 16 kHz can hold, and would come back as 6 kHz.
    const float peak = settledPeak(resampled(sine(10000, 48000, 1), 48000));
    QVERIFY2(peak < 0.005, qPrintable(QString::number(peak)));
}

void tst_VoiceInput::resamplerWorksInChunks()
{
    const std::vector<float> input = sine(1000, 44100, 0.5);
    const std::vector<float> whole = resampled(input, 44100);
    Resampler resampler(44100, kSampleRate);
    std::vector<float> chunked;
    for (std::size_t start = 0; start < input.size(); start += 441)
        resampler.process(input.data() + start, std::min<std::size_t>(441, input.size() - start),
                          chunked);
    QCOMPARE(chunked.size(), whole.size());
    for (std::size_t i = 0; i < whole.size(); ++i)
        QVERIFY2(std::abs(chunked[i] - whole[i]) < 1e-5, qPrintable(QString::number(i)));
}

void tst_VoiceInput::wavFile()
{
    const QByteArray wav = VoiceInput::Internal::wavFile({0.0f, 0.5f, -1.0f, 2.0f}, kSampleRate);
    QCOMPARE(wav.size(), 44 + 4 * 2);
    QCOMPARE(wav.left(4), "RIFF");
    QCOMPARE(qFromLittleEndian<quint32>(wav.constData() + 4), quint32(wav.size() - 8));
    QCOMPARE(wav.mid(8, 8), "WAVEfmt ");
    QCOMPARE(qFromLittleEndian<quint16>(wav.constData() + 22), quint16(1));
    QCOMPARE(qFromLittleEndian<quint32>(wav.constData() + 24), quint32(kSampleRate));
    QCOMPARE(qFromLittleEndian<quint16>(wav.constData() + 34), quint16(16));
    QCOMPARE(wav.mid(36, 4), "data");
    QCOMPARE(qFromLittleEndian<quint32>(wav.constData() + 40), quint32(4 * 2));
    const QList<qint16> samples{0, 16384, -32767, 32767};
    for (int i = 0; i < samples.size(); ++i)
        QCOMPARE(qFromLittleEndian<qint16>(wav.constData() + 44 + 2 * i), samples.at(i));
}

QTEST_GUILESS_MAIN(tst_VoiceInput)

#include "tst_voiceinput.moc"
