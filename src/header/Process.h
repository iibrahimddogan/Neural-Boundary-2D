#pragma once
#include <vector>

using namespace std;

vector<float> init_array_random(int lenght);
void Z_Score_Parameters(const vector<float>& samples,vector<float>& mean, vector<float>& std);
int Test_Forward(const vector<float>& x, const vector<float>& weight, const vector<float>& bias, int num_Class);
