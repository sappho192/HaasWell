#ifndef HAASWELL_OUTPUT_H
#define HAASWELL_OUTPUT_H
#include <cmath>

namespace haaswell {
    template <typename Sample>
    void ProcessOutput(int nFrames, const int nChans, Sample** inputs, Sample** outputs, const double dB)
    {
        const Sample factor = static_cast<Sample>(std::pow(10.0, dB / 20));
        for (int s = 0; s < nFrames; s++) {
            for (int c = 0; c < nChans; c++) {
                outputs[c][s] = inputs[c][s] * factor; // 10 ^ (dB / 20)
            }
        }
    }
}
#endif
