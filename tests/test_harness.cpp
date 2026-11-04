// Self-test for tests/harness.h: checks that the harness itself reports
// passes and failures correctly before it is trusted with the DSP tests.
// Written by Claude for the study course.
//
// Build & run (from the repo root):
//   g++ -std=c++17 -O2 -Wall -Wextra -Wconversion tests/test_harness.cpp -o test_harness.exe
//   ./test_harness.exe

#include "harness.h"

namespace {

void CountsPassesAndFailures()
{
    test::Section("Check counts passes and failures");
    std::printf("(the two lines below are intentional: one PASS, one FAIL)\n");
    test::Check(true, "intentional pass");
    test::Check(false, "intentional FAIL - expected");
    const test::Results seen = test::GetResults();
    test::ResetResults();

    test::Check(seen.passed == 1 && seen.failed == 1, "one pass and one failure were counted",
                test::Fmt("passed %d, failed %d", seen.passed, seen.failed));
}

void SummaryReturnsNonZeroOnFailure()
{
    test::Section("Summary exit code");
    const test::Results before = test::GetResults();

    test::ResetResults();
    test::GetResults().failed = 1;
    std::printf("(the summary line below is intentional)\n");
    const int withFailure = test::Summary();

    test::ResetResults();
    test::GetResults().passed = 1;
    std::printf("(the summary line below is intentional)\n");
    const int allPassed = test::Summary();

    test::GetResults() = before;
    test::Check(withFailure != 0, "non-zero when something failed");
    test::Check(allPassed == 0, "zero when everything passed");
}

void CheckNearUsesTheTolerance()
{
    test::Section("CheckNear");
    const test::Results before = test::GetResults();
    std::printf("(the three lines below are intentional: PASS, FAIL, FAIL)\n");
    const bool inside = test::CheckNear(1.0005, 1.0, 1e-3, "intentional: inside tolerance");
    const bool outside = test::CheckNear(1.002, 1.0, 1e-3, "intentional: outside tolerance - expected FAIL");
    const bool nan = test::CheckNear(std::nan(""), 1.0, 1e9, "intentional: NaN - expected FAIL");
    test::GetResults() = before;

    test::Check(inside, "a value inside the tolerance passes");
    test::Check(!outside, "a value outside the tolerance fails");
    test::Check(!nan, "NaN fails even with a huge tolerance");
}

void BuffersHaveThePluginShape()
{
    test::Section("Buffers");
    test::Buffers<float> b(2, 8);
    float** p = b.Ptrs();
    test::Check(b.NumChannels() == 2 && b.NumFrames() == 8, "size is 2 channels x 8 frames");
    test::Check(test::Peak(b[0]) == 0.0 && test::Peak(b[1]) == 0.0, "starts silent");

    p[1][3] = 0.25f;
    test::Check(b[1][3] == 0.25f, "Ptrs() writes land in the channel data");

    test::Buffers<float> copy = b;
    copy[1][3] = -1.0f;
    copy.Ptrs()[0][0] = -1.0f;
    test::Check(b[1][3] == 0.25f && b[0][0] == 0.0f, "a copy owns its samples (deep copy)");
    test::Check(copy.Ptrs()[1] != b.Ptrs()[1], "a copy has its own pointer array");
}

void SignalsAreDeterministic()
{
    test::Section("Signals");
    std::vector<double> a(1000), b(1000), c(1000);
    test::FillRandom(a, 7, 0.5);
    test::FillRandom(b, 7, 0.5);
    test::FillRandom(c, 8, 0.5);
    test::Check(test::CountNotEqual(a, b) == 0, "same seed gives the same noise");
    test::Check(test::CountNotEqual(a, c) > 0, "another seed gives other noise");
    test::Check(test::Peak(a) <= 0.5, "noise stays within the amplitude", test::Fmt("peak %.6f", test::Peak(a)));

    test::Buffers<double> stereo(2, 100);
    test::FillRandom(stereo, 1);
    test::Check(test::CountNotEqual(stereo[0], stereo[1]) > 0, "each channel gets different noise");

    std::vector<double> sine(48000);
    test::FillSine(sine, 1000.0, 48000.0, 0.5);
    test::CheckNear(sine[12], 0.5, 1e-12, "1 kHz sine at fs 48 kHz peaks at sample 12 (a quarter period)");
    test::CheckNear(test::Rms(sine), 0.5 / std::sqrt(2.0), 1e-9, "RMS of a sine is amplitude / sqrt(2)");
}

void MeasurementsMatchKnownValues()
{
    test::Section("Measurements");
    test::CheckNear(test::ToDb(0.5), -6.0206, 1e-4, "ToDb(0.5) is about -6.02 dB");
    test::CheckNear(test::ToDb(1.0), 0.0, 0.0, "ToDb(1) is exactly 0 dB");
    test::Check(test::ToDb(0.0) == -240.0, "ToDb(0) is floored to -240 dB instead of -inf");

    const std::vector<float> x {1.f, 2.f, 3.f};
    const std::vector<float> y {1.f, 2.5f, 3.f};
    const std::vector<float> longer {1.f, 2.f, 3.f, 4.f};
    test::Check(test::CountNotEqual(x, x) == 0, "a buffer equals itself");
    test::Check(test::CountNotEqual(x, y) == 1, "one changed sample is counted once");
    test::Check(test::CountNotEqual(x, longer) == 1, "an extra sample counts as a difference");
    test::CheckNear(test::MaxAbsDiff(x, y), 0.5, 0.0, "MaxAbsDiff finds the largest difference");
    test::CheckNear(test::Rms(std::vector<double>{}), 0.0, 0.0, "RMS of an empty buffer is 0");
}

} // namespace

int main()
{
    CountsPassesAndFailures();
    SummaryReturnsNonZeroOnFailure();
    CheckNearUsesTheTolerance();
    BuffersHaveThePluginShape();
    SignalsAreDeterministic();
    MeasurementsMatchKnownValues();
    return test::Summary();
}
