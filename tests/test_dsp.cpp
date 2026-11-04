#include <algorithm>

#include "../dsp/output.h"
#include "harness.h"
using test::Section;
using test::Check;
using haaswell::ProcessOutput;

template <typename T>
void OutputTest1();
template <typename T>
void OutputTest2();
template <typename T>
void OutputTest3();
template <typename T>
void OutputTest4();
template <typename T>
void OutputTest5();
template <typename T>
void RunTest();

int main(void) {
	Section("Output (double)");
	RunTest<double>();

	Section("Output (float)");
	RunTest<float>();

	return test::Summary();
}

template <typename T>
void RunTest()
{
	OutputTest1<T>();
	OutputTest2<T>();
	OutputTest3<T>();
	OutputTest4<T>();
	OutputTest5<T>();
}

/// <summary>
/// Test: 0dB for ProcessOutput().
/// Condition: Input and output must be same when ProcessOutput() is applied
/// </summary>
template <typename T>
void OutputTest1()
{
	auto inBuffers = test::Buffers<T>(2, 512);
	test::FillRandom(inBuffers, 42);
	auto outBuffers = test::Buffers<T>(2, 512);
	ProcessOutput(inBuffers.NumFrames(), inBuffers.NumChannels(), inBuffers.Ptrs(), outBuffers.Ptrs(), 0.0);

	for (int i = 0; i < inBuffers.NumChannels(); i++) {
		auto result = test::CountNotEqual(inBuffers[i], outBuffers[i]);
		Check(result == 0,
			test::Fmt("[Test 1] (Channel %d)", i),
			test::Fmt("ProcessOutput(Random signal, 0dB): %zu different sample(s)", result));
	}
}

/// <summary>
/// Test: Check if -6.0206 dB makes amplitude as half
/// Condition: Output/Input == 0.5 (error <= 1e-6)
/// </summary>
template <typename T>
void OutputTest2()
{
	auto inBuffers = test::Buffers<T>(2, 512);
	test::FillRandom(inBuffers, 42);
	auto outBuffers = test::Buffers<T>(2, 512);
	ProcessOutput(inBuffers.NumFrames(), inBuffers.NumChannels(), inBuffers.Ptrs(), outBuffers.Ptrs(), -6.0206);

	for (int i = 0; i < inBuffers.NumChannels(); i++) {
		for (auto &sample : inBuffers[i]) {
			sample = sample / 2;
		}
		auto maxAbsDiff = test::MaxAbsDiff(inBuffers[i], outBuffers[i]);
		test::CheckNear(maxAbsDiff, 0, 1e-6, test::Fmt("[Test 2][Channel %d] Output/Input == 0.5 (error <= 1e-6)", i));
	}
}

/// <summary>
/// Test: Check if 12 dB makes amplitude as quadruple
/// Condition: Output/Input == 3.981 (error <= 1e-3)
/// </summary>
template <typename T>
void OutputTest3()
{
	auto inBuffers = test::Buffers<T>(2, 512);
	test::FillRandom(inBuffers, 42);
	auto outBuffers = test::Buffers<T>(2, 512);
	ProcessOutput(inBuffers.NumFrames(), inBuffers.NumChannels(), inBuffers.Ptrs(), outBuffers.Ptrs(), 12.0);

	for (int i = 0; i < inBuffers.NumChannels(); i++) {
		for (auto& sample : inBuffers[i]) {
			sample = sample * static_cast<T>(3.981);
		}
		auto maxAbsDiff = test::MaxAbsDiff(inBuffers[i], outBuffers[i]);
		test::CheckNear(maxAbsDiff, 0, 1e-3, test::Fmt("[Test 3][Channel %d] Output/Input == 3.981 (error <= 1e-3)", i));
	}
}

/// <summary>
/// Test: Channel should not be mixed
/// Condition: Only left channel has signal, right channel contains silence. The output of right channel must be 0.
/// </summary>
template <typename T>
void OutputTest4()
{
	auto inBuffers = test::Buffers<T>(2, 512);
	test::FillRandom(inBuffers[0], 42); // Left channel
	auto outBuffers = test::Buffers<T>(2, 512);
	std::fill(outBuffers[0].begin(), outBuffers[0].end(), static_cast<T>(1));
	std::fill(outBuffers[1].begin(), outBuffers[1].end(), static_cast<T>(2));

	ProcessOutput(inBuffers.NumFrames(), inBuffers.NumChannels(), inBuffers.Ptrs(), outBuffers.Ptrs(), 12.0);
	auto result = test::Peak(outBuffers[1]);
	Check(result == 0.0, "[Test 4] Channel mix test (L->R)", test::Fmt("Peak level of right channel: %.9g", result));

	test::FillRandom(inBuffers[1], 43);
	std::fill(inBuffers[0].begin(), inBuffers[0].end(), static_cast<T>(0));
	std::fill(outBuffers[0].begin(), outBuffers[0].end(), static_cast<T>(1));
	std::fill(outBuffers[1].begin(), outBuffers[1].end(), static_cast<T>(2));

	ProcessOutput(inBuffers.NumFrames(), inBuffers.NumChannels(), inBuffers.Ptrs(), outBuffers.Ptrs(), 12.0);
	result = test::Peak(outBuffers[0]);
	Check(result == 0.0, "[Test 4] Channel mix test (R->L)", test::Fmt("Peak level of left channel: %.9g", result));
}

/// <summary>
/// Test: In-place processing
/// Condition: Same result even when input and output share the same buffer memory.
/// </summary>
template <typename T>
void OutputTest5()
{
	auto inBuffers = test::Buffers<T>(2, 512);
	test::FillRandom(inBuffers, 42);
	test::Buffers<T> inPlaceBuffers(inBuffers); // Calling copy constructor

	auto outBuffers = test::Buffers<T>(2, 512);

	ProcessOutput(inBuffers.NumFrames(), inBuffers.NumChannels(), inBuffers.Ptrs(), outBuffers.Ptrs(), -3.0);
	ProcessOutput(inPlaceBuffers.NumFrames(), inPlaceBuffers.NumChannels(), inPlaceBuffers.Ptrs(), inPlaceBuffers.Ptrs(), -3.0);

	for (int i = 0; i < inPlaceBuffers.NumChannels(); i++) {
		auto maxAbsDiff = test::MaxAbsDiff(inPlaceBuffers[i], outBuffers[i]);
		Check(maxAbsDiff == 0.0, test::Fmt("[Test 5] In-place processing (Channel %d)", i),
			test::Fmt("Difference between in-place and separated buffer processing: %.9g", maxAbsDiff));
	}
}
