#include "Process.h"
#include <cmath>
#include <cstdlib>
#include <vector>


vector<float> init_array_random(int lenght){

    vector<float> randomArr(lenght);
    for (int i = 0; i < lenght; i++) {
        randomArr[i] = ((float)rand() / RAND_MAX) - 0.5f;
    }
    return randomArr;
};


void Z_Score_Parameters(const vector<float>& samples,vector<float>& mean, vector<float>& std){
    mean.assign(2,0.0f);
    std.assign(2, 0.0f);

    int size{static_cast<int>(samples.size() / 2)};


    for (size_t i = 0; i < samples.size(); i+=2) {
        mean[0] += samples[i];
        mean[1] += samples[ i + 1 ];
    }

    mean[0] = mean[0] / size;
    mean[1] = mean[1] / size;

    for (size_t i = 0; i < samples.size(); i += 2) {
        std[0] += (samples[i] - mean[0]) * (samples[i] - mean[0]);
        std[1] += (samples[i + 1] - mean[1]) * (samples[i + 1] - mean[1]);
    }

    std[0] = sqrt(std[0] / size);
    std[1] = sqrt(std[1] / size);
}

int Test_Forward(const vector<float>& x, const vector<float>& weight, const vector<float>& bias, int num_Class){
    int index_Max { 0 };

    if (num_Class > 2) {
        vector<float> output(num_Class, 0.0f);

        for (int i = 0; i < num_Class; i++) {
            float a;
            a = x[0] * weight[i * 2];
            a += (x[1] * weight[i * 2 + 1 ]) + bias[i];
            output[i] = tanh(a);
        }
        float max { output[0]};

        for (int i = 1; i < num_Class; i++) {
            if (output[i] > max) {
                max = output[i];
                index_Max = i;
            }
        }
    }
    else {
        float output { 0.0f };
        output = tanh((x[0] * weight[0]) + (x[1] * weight[1]) + bias[0]);
        if (output > 0) {
            index_Max = 0;
        }
        else {
            index_Max = 1;
        }
    }
    return index_Max;
}
