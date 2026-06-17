#pragma once

#include "AudioFormat.h"

#include <QByteArray>
#include <QtMath>
#include <cmath>
#include <cstdint>
#include <vector>

// Test helpers for generating and comparing PCM signals.
namespace testutil {

// Generates `samples` of a mono int16 sine wave at the given frequency.
inline std::vector<int16_t> sine(int samples, double freqHz = 440.0,
                                 double amplitude = 0.5)
{
    std::vector<int16_t> pcm(samples);
    for (int i = 0; i < samples; ++i) {
        const double t = double(i) / sound::kSampleRate;
        const double v = amplitude * std::sin(2.0 * M_PI * freqHz * t);
        pcm[i] = int16_t(qBound(-1.0, v, 1.0) * 32767.0);
    }
    return pcm;
}

inline QByteArray toBytes(const std::vector<int16_t>& pcm)
{
    return QByteArray(reinterpret_cast<const char*>(pcm.data()),
                      int(pcm.size() * sizeof(int16_t)));
}

inline double rms(const int16_t* pcm, int n)
{
    if (n <= 0)
        return 0.0;
    double acc = 0.0;
    for (int i = 0; i < n; ++i) {
        const double s = pcm[i] / 32768.0;
        acc += s * s;
    }
    return std::sqrt(acc / n);
}

inline double rms(const std::vector<int16_t>& pcm)
{
    return rms(pcm.data(), int(pcm.size()));
}

// Normalized cross-correlation in [-1, 1] over the overlapping prefix.
// Robust to the codec delay-free, equal-length case used in the tests.
inline double correlation(const std::vector<int16_t>& a,
                          const std::vector<int16_t>& b)
{
    const int n = int(std::min(a.size(), b.size()));
    if (n == 0)
        return 0.0;

    double sa = 0, sb = 0;
    for (int i = 0; i < n; ++i) { sa += a[i]; sb += b[i]; }
    const double ma = sa / n, mb = sb / n;

    double num = 0, da = 0, db = 0;
    for (int i = 0; i < n; ++i) {
        const double xa = a[i] - ma;
        const double xb = b[i] - mb;
        num += xa * xb;
        da  += xa * xa;
        db  += xb * xb;
    }
    if (da <= 0 || db <= 0)
        return 0.0;
    return num / std::sqrt(da * db);
}

// Best correlation of `out` against `ref` over lags [0, maxLag], to tolerate
// the codec's algorithmic delay (Opus adds ~6.5 ms lookahead at 48 kHz).
inline double bestCorrelation(const std::vector<int16_t>& ref,
                              const std::vector<int16_t>& out,
                              int maxLag = 1500)
{
    double best = -1.0;
    for (int lag = 0; lag <= maxLag && lag < int(out.size()); ++lag) {
        std::vector<int16_t> shifted(out.begin() + lag, out.end());
        best = std::max(best, correlation(ref, shifted));
    }
    return best;
}

} // namespace testutil
