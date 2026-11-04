#include <algorithm>

#include "../dsp/output.h"
#include "harness.h"
using test::Section;
using test::Check;
using haaswell::ProcessOutput;

void OutputTest1();
void OutputTest2();
void OutputTest3();
void OutputTest4();

int main(void) {
	Section("Output");

	OutputTest1();
	OutputTest2();
	OutputTest3();
	OutputTest4();

	return test::Summary();
}

/// <summary>
/// Test: 0dB for ProcessOutput().
/// Condition: Input and output must be same when ProcessOutput() is applied
/// </summary>
void OutputTest1()
{
	auto inBuffers = test::Buffers<double>(2, 512);
	test::FillRandom(inBuffers, 42);
	auto outBuffers = test::Buffers<double>(2, 512);
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
void OutputTest2()
{
	auto inBuffers = test::Buffers<double>(2, 512);
	test::FillRandom(inBuffers, 42);
	auto outBuffers = test::Buffers<double>(2, 512);
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
void OutputTest3()
{
	auto inBuffers = test::Buffers<double>(2, 512);
	test::FillRandom(inBuffers, 42);
	auto outBuffers = test::Buffers<double>(2, 512);
	ProcessOutput(inBuffers.NumFrames(), inBuffers.NumChannels(), inBuffers.Ptrs(), outBuffers.Ptrs(), 12.0);

	for (int i = 0; i < inBuffers.NumChannels(); i++) {
		for (auto& sample : inBuffers[i]) {
			sample = sample * 3.981;
		}
		auto maxAbsDiff = test::MaxAbsDiff(inBuffers[i], outBuffers[i]);
		test::CheckNear(maxAbsDiff, 0, 1e-3, test::Fmt("[Test 3][Channel %d] Output/Input == 3.981 (error <= 1e-3)", i));
	}
}

/// <summary>
/// Test: Channel should not be mixed
/// Condition: Only left channel has signal, right channel contains silence. The output of right channel must be 0.
/// </summary>
void OutputTest4()
{
	auto inBuffers = test::Buffers<double>(2, 512);
	test::FillRandom(inBuffers[0], 42); // Left channel
	auto outBuffers = test::Buffers<double>(2, 512);
	std::fill(outBuffers[0].begin(), outBuffers[0].end(), 1);
	std::fill(outBuffers[1].begin(), outBuffers[1].end(), 2);

	ProcessOutput(inBuffers.NumFrames(), inBuffers.NumChannels(), inBuffers.Ptrs(), outBuffers.Ptrs(), 12.0);
	auto result = test::Peak(outBuffers[1]);
	Check(result == 0.0, "[Test 4] Channel mix test (L->R)", test::Fmt("Peak level of right channel: %.9g", result));

	test::FillRandom(inBuffers[1], 43);
	std::fill(inBuffers[0].begin(), inBuffers[0].end(), 0);
	std::fill(outBuffers[0].begin(), outBuffers[0].end(), 1);
	std::fill(outBuffers[1].begin(), outBuffers[1].end(), 2);

	ProcessOutput(inBuffers.NumFrames(), inBuffers.NumChannels(), inBuffers.Ptrs(), outBuffers.Ptrs(), 12.0);
	result = test::Peak(outBuffers[0]);
	Check(result == 0.0, "[Test 4] Channel mix test (R->L)", test::Fmt("Peak level of left channel: %.9g", result));
}
