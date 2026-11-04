#pragma once

// Minimal test harness for the framework-independent DSP in ../dsp.
// Standard library only, so any test file builds with one line:
//
//   g++ -std=c++17 -O2 -Wall -Wextra -Wconversion tests/<file>.cpp -o <file>.exe
//
// Written by Claude for the study course: the harness is infrastructure,
// the test cases are the student's.
//
// Usage:
//
//   #include "harness.h"
//
//   int main()
//   {
//       test::Section("Example");
//       test::Check(1 + 1 == 2, "addition works");
//       test::CheckNear(0.1 + 0.2, 0.3, 1e-12, "close enough");
//       return test::Summary();   // non-zero if anything failed
//   }
//
// There is deliberately no dB -> gain helper here. Write the expected value
// into the test itself (e.g. 0.5 for -6.0206 dB), so a test never checks a
// formula by reusing the same formula.

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

// Lets GCC/Clang check Fmt's format string like printf's. On MinGW, plain `printf` here means
// the old MS checker, which rejects %zu even though the runtime prints it; __MINGW_PRINTF_FORMAT
// names the checker that matches the stdio actually linked.
#if defined(__MINGW32__) && defined(__MINGW_PRINTF_FORMAT)
#define TEST_PRINTF_FORMAT(fmtIndex, firstArg) __attribute__((format(__MINGW_PRINTF_FORMAT, fmtIndex, firstArg)))
#elif defined(__GNUC__)
#define TEST_PRINTF_FORMAT(fmtIndex, firstArg) __attribute__((format(printf, fmtIndex, firstArg)))
#else
#define TEST_PRINTF_FORMAT(fmtIndex, firstArg)
#endif

namespace test {

// ---------------------------------------------------------------------------
// Results
// ---------------------------------------------------------------------------

struct Results
{
    int passed = 0;
    int failed = 0;
};

inline Results& GetResults()
{
    static Results results;
    return results;
}

/** Clears the pass/fail counts. Only the harness self-test needs this. */
inline void ResetResults() { GetResults() = Results{}; }

/** printf-style formatting into a std::string, for Check details. */
inline std::string Fmt(const char* fmt, ...) TEST_PRINTF_FORMAT(1, 2);

inline std::string Fmt(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    va_list measure;
    va_copy(measure, args);
    const int length = std::vsnprintf(nullptr, 0, fmt, measure);
    va_end(measure);

    std::string out;
    if (length > 0)
    {
        std::vector<char> buffer(static_cast<size_t>(length) + 1);
        std::vsnprintf(buffer.data(), buffer.size(), fmt, args);
        out.assign(buffer.data(), static_cast<size_t>(length));
    }
    va_end(args);
    return out;
}

/** Prints a heading, to group the checks that follow. */
inline void Section(const std::string& name) { std::printf("\n== %s ==\n", name.c_str()); }

/** Records one check and prints one line. Returns ok, so a test can stop early on failure. */
inline bool Check(bool ok, const std::string& name, const std::string& detail = "")
{
    std::printf("[%s] %s", ok ? "PASS" : "FAIL", name.c_str());
    if (!detail.empty())
        std::printf("  (%s)", detail.c_str());
    std::printf("\n");

    if (ok)
        ++GetResults().passed;
    else
        ++GetResults().failed;
    return ok;
}

/** Passes when |actual - expected| <= tolerance. NaN always fails. */
inline bool CheckNear(double actual, double expected, double tolerance, const std::string& name)
{
    const double diff = std::abs(actual - expected);
    const bool ok = diff <= tolerance;  // false for NaN
    return Check(ok, name, Fmt("actual %.9g, expected %.9g, |diff| %.3g, tolerance %.3g", actual, expected, diff, tolerance));
}

/** Prints the totals. Return this from main(): 0 when everything passed. */
inline int Summary()
{
    const Results& r = GetResults();
    std::printf("\n%d passed, %d failed\n", r.passed, r.failed);
    return r.failed == 0 ? 0 : 1;
}

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

/** "float", "double", ... for check names in tests templated on the sample type. */
template <typename T>
const char* TypeName()
{
    if constexpr (std::is_same_v<T, float>)
        return "float";
    else if constexpr (std::is_same_v<T, double>)
        return "double";
    else if constexpr (std::is_same_v<T, long double>)
        return "long double";
    else
        return "?";
}

// ---------------------------------------------------------------------------
// Buffers
// ---------------------------------------------------------------------------

/** Multichannel audio buffers in the T** shape that plugins pass around
 *  (one array per channel, plus an array of pointers to them).
 *
 *    test::Buffers<float> in(2, 512);   // 2 channels x 512 frames, all zero
 *    in[0][10] = 0.5f;                  // channel 0, frame 10
 *    Process(in.Ptrs(), ...);           // float**
 *
 *  Copies are deep: a copy owns its own samples and its own pointer array,
 *  which is handy for keeping the original input of an in-place test.
 */
template <typename T>
class Buffers
{
public:
    Buffers(int numChannels, int numFrames)
    : mData(static_cast<size_t>(numChannels), std::vector<T>(static_cast<size_t>(numFrames), T(0)))
    {
        RebuildPtrs();
    }

    Buffers(const Buffers& other)
    : mData(other.mData)
    {
        RebuildPtrs();
    }

    Buffers& operator=(const Buffers& other)
    {
        mData = other.mData;
        RebuildPtrs();
        return *this;
    }

    T** Ptrs() { return mPtrs.data(); }

    int NumChannels() const { return static_cast<int>(mData.size()); }
    int NumFrames() const { return mData.empty() ? 0 : static_cast<int>(mData[0].size()); }

    std::vector<T>& operator[](int channel) { return mData[static_cast<size_t>(channel)]; }
    const std::vector<T>& operator[](int channel) const { return mData[static_cast<size_t>(channel)]; }

private:
    void RebuildPtrs()
    {
        mPtrs.resize(mData.size());
        for (size_t c = 0; c < mData.size(); ++c)
            mPtrs[c] = mData[c].data();
    }

    std::vector<std::vector<T>> mData;
    std::vector<T*> mPtrs;
};

// ---------------------------------------------------------------------------
// Signals
// ---------------------------------------------------------------------------

/** Uniform noise in [-amplitude, amplitude]. The same seed always gives the same samples. */
template <typename T>
void FillRandom(std::vector<T>& x, unsigned seed, double amplitude = 0.5)
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(-amplitude, amplitude);
    for (T& v : x)
        v = static_cast<T>(dist(rng));
}

/** Fills every channel with noise; channel c uses seed + c, so channels differ. */
template <typename T>
void FillRandom(Buffers<T>& buffers, unsigned seed, double amplitude = 0.5)
{
    for (int c = 0; c < buffers.NumChannels(); ++c)
        FillRandom(buffers[c], seed + static_cast<unsigned>(c), amplitude);
}

/** amplitude * sin(2*pi*freq*n/fs + phase), for n = 0, 1, 2, ... */
template <typename T>
void FillSine(std::vector<T>& x, double freqHz, double sampleRate, double amplitude = 0.5, double phaseRad = 0.0)
{
    constexpr double kTwoPi = 6.283185307179586476925286766559;
    for (size_t n = 0; n < x.size(); ++n)
        x[n] = static_cast<T>(amplitude * std::sin(kTwoPi * freqHz * static_cast<double>(n) / sampleRate + phaseRad));
}

// ---------------------------------------------------------------------------
// Measurements
// ---------------------------------------------------------------------------

/** Root mean square of x[from..end). 0 for an empty range. */
template <typename T>
double Rms(const std::vector<T>& x, size_t from = 0)
{
    if (from >= x.size())
        return 0.0;
    double sum = 0.0;
    for (size_t n = from; n < x.size(); ++n)
        sum += static_cast<double>(x[n]) * static_cast<double>(x[n]);
    return std::sqrt(sum / static_cast<double>(x.size() - from));
}

/** Largest |x[n]|. */
template <typename T>
double Peak(const std::vector<T>& x)
{
    double peak = 0.0;
    for (const T& v : x)
        peak = std::max(peak, std::abs(static_cast<double>(v)));
    return peak;
}

/** Amplitude ratio -> dB (20 * log10). Ratios at or below 1e-12 read as -240 dB instead of -inf. */
inline double ToDb(double amplitudeRatio) { return 20.0 * std::log10(std::max(amplitudeRatio, 1e-12)); }

/** How many samples differ when compared with ==. Different lengths count the extra samples too.
 *  Note: == compares values, so -0.0 equals +0.0, and NaN never equals anything (not even itself). */
template <typename T>
size_t CountNotEqual(const std::vector<T>& a, const std::vector<T>& b)
{
    const size_t common = std::min(a.size(), b.size());
    size_t count = std::max(a.size(), b.size()) - common;
    for (size_t n = 0; n < common; ++n)
        if (a[n] != b[n])
            ++count;
    return count;
}

/** Largest |a[n] - b[n]| over the common length. */
template <typename T>
double MaxAbsDiff(const std::vector<T>& a, const std::vector<T>& b)
{
    const size_t common = std::min(a.size(), b.size());
    double worst = 0.0;
    for (size_t n = 0; n < common; ++n)
        worst = std::max(worst, std::abs(static_cast<double>(a[n]) - static_cast<double>(b[n])));
    return worst;
}

} // namespace test
