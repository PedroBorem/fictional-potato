#include "xgboost_error_postprocess.h"

double xgboost_predict_error_probability(double *features)
{
    double output[2] = {0.0, 0.0};

    xgboost_model_score(features, output);

    return output[1];
}

bool xgboost_predict_has_error(double *features)
{
    return xgboost_predict_error_probability(features) >= XGBOOST_ERROR_THRESHOLD;
}
