/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file xcs_supervised.c
 * @author Richard Preen <rpreen@gmail.com>
 * @copyright The Authors.
 * @date 2015--2020.
 * @brief Supervised regression learning functions.
 */

#include "xcs_supervised.h"
#include "clset.h"
#include "ea.h"
#include "loss.h"
#include "pa.h"
#include "param.h"
#include "perf.h"
#include "utils.h"

/**
 * @brief Selects a data sample for training or testing.
 * @param data The input data.
 * @param cnt The current sequence counter.
 * @param shuffle Whether to select the sample randomly.
 * @return The row of the data sample selected.
 */
static int
xcs_supervised_sample(const struct INPUT *data, const int cnt,
                      const _Bool shuffle)
{
    if (shuffle) {
        return irand_uniform(0, data->n_samples);
    }
    return cnt % data->n_samples;
}

/**
 * @brief Executes a single XCSF trial.
 * @param xcsf The XCSF data structure.
 * @param x The feature variables.
 * @param y The labelled variables.
 */
static void
xcs_supervised_trial(struct XCSF *xcsf, const double *x, const double *y)
{
    clset_init(&xcsf->mset);
    clset_init(&xcsf->kset);
    clset_match(xcsf, x);
    pa_build(xcsf, x);
    if (xcsf->explore) {
        clset_update(xcsf, &xcsf->mset, x, y, true);
        ea(xcsf, &xcsf->mset);
    }
    clset_kill(xcsf, &xcsf->kset);
    clset_free(&xcsf->mset);
}

/**
 * @brief Executes MAX_TRIALS number of XCSF learning iterations using the
 * training data and test iterations using the test data.
 * @param xcsf The XCSF data structure.
 * @param train_data The input data to use for training.
 * @param test_data The input data to use for testing.
 * @param shuffle Whether to randomise the instances during training.
 * @return The average XCSF training error using the loss function.
 */
double
xcs_supervised_fit(struct XCSF *xcsf, const struct INPUT *train_data,
                   const struct INPUT *test_data, const _Bool shuffle)
{
    double err = 0; // training error: total over all trials
    double werr = 0; // training error: windowed total
    double wterr = 0; // testing error: windowed total
    for (int cnt = 0; cnt < xcsf->MAX_TRIALS; ++cnt) {
        // training sample
        int row = xcs_supervised_sample(train_data, cnt, shuffle);
        const double *x = &train_data->x[row * train_data->x_dim];
        const double *y = &train_data->y[row * train_data->y_dim];
        param_set_explore(xcsf, true);
        xcs_supervised_trial(xcsf, x, y);
        const double error = (xcsf->loss_ptr)(xcsf, xcsf->pa, y);
        werr += error;
        err += error;
        xcsf->error += (error - xcsf->error) * xcsf->BETA;
        // test sample
        if (test_data != NULL) {
            row = xcs_supervised_sample(test_data, cnt, shuffle);
            x = &test_data->x[row * test_data->x_dim];
            y = &test_data->y[row * test_data->y_dim];
            param_set_explore(xcsf, false);
            xcs_supervised_trial(xcsf, x, y);
            wterr += (xcsf->loss_ptr)(xcsf, xcsf->pa, y);
        }
        perf_print(xcsf, &werr, &wterr, cnt);
    }
    return err / xcsf->MAX_TRIALS;
}

/**
 * @brief Calculates the XCSF predictions for the provided input.
 * @param xcsf The XCSF data structure.
 * @param x The input feature variables.
 * @param pred The calculated XCSF predictions (set by this function).
 * @param n_samples The number of instances.
 */
void
xcs_supervised_predict(struct XCSF *xcsf, const double *x, double *pred,
                       const int n_samples)
{
    param_set_explore(xcsf, false);
    for (int row = 0; row < n_samples; ++row) {
        xcs_supervised_trial(xcsf, &x[row * xcsf->x_dim], NULL);
        memcpy(&pred[row * xcsf->y_dim], xcsf->pa,
               sizeof(double) * xcsf->y_dim);
    }
}

/**
 * @brief Calculates the XCSF error for the input data.
 * @param xcsf The XCSF data structure.
 * @param data The input data to calculate the error.
 * @return The average XCSF error using the loss function.
 */
double
xcs_supervised_score(struct XCSF *xcsf, const struct INPUT *data)
{
    param_set_explore(xcsf, false);
    double err = 0;
    for (int row = 0; row < data->n_samples; ++row) {
        const double *x = &data->x[row * data->x_dim];
        const double *y = &data->y[row * data->y_dim];
        xcs_supervised_trial(xcsf, x, y);
        err += (xcsf->loss_ptr)(xcsf, xcsf->pa, y);
    }
    return err / data->n_samples;
}

/**
 * @brief Calculates the XCSF error for a subsample of the input data.
 * @param xcsf The XCSF data structure.
 * @param data The input data to calculate the error.
 * @param N The number of subsamples to draw randomly for scoring.
 * @return The average XCSF error using the loss function.
 */
double
xcs_supervised_score_n(struct XCSF *xcsf, const struct INPUT *data, const int N)
{
    param_set_explore(xcsf, false);
    double err = 0;
    for (int i = 0; i < N; ++i) {
        const int row = xcs_supervised_sample(data, i, true);
        const double *x = &data->x[row * data->x_dim];
        const double *y = &data->y[row * data->y_dim];
        xcs_supervised_trial(xcsf, x, y);
        err += (xcsf->loss_ptr)(xcsf, xcsf->pa, y);
    }
    return err / N;
}
