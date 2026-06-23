#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XGBOOST_ERROR_THRESHOLD 0.47999999999999998

void xgboost_model_score(double *input, double *output);
double xgboost_predict_error_probability(double *features);
bool xgboost_predict_has_error(double *features);

#ifdef __cplusplus
}
#endif
