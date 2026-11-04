#include "../dsp/output.h"
#include "harness.h"
using test::Section;
using test::Check;
using haaswell::ProcessOutput;

void OutputTest1();

int main(void) {
	Section("Output");

	OutputTest1();

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
			test::Fmt("Test 1 (Channel %d)", i),
			test::Fmt("ProcessOutput(Random signal, 0dB): %zu different sample(s)", result));
	}
}

