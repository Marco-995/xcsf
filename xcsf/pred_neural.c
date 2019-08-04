/*
 * Copyright (C) 2016--2019 Richard Preen <rpreen@gmail.com>
 *
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
 *
 **************
 * Description: 
 **************
 * The MLP neural network classifier computed prediction module.
 *
 * Creates a weight vector representing an MLP neural network to calculate the
 * expected value given a problem instance and adapts the weights using the
 * backpropagation algorithm.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "random.h"
#include "cons.h"
#include "cl.h"
#include "neural.h"
#include "pred_neural.h"
 
typedef struct PRED_NEURAL {
	BPN bpn;
	double *pre;
} PRED_NEURAL;
 
void pred_neural_init(CL *c)
{
	PRED_NEURAL *pred = malloc(sizeof(PRED_NEURAL));
	int neurons[3] = {num_x_vars, NUM_HIDDEN_NEURONS, num_y_vars};
	double (*activations[2])(double) = {sig, sig};
	neural_init(&pred->bpn, 3, neurons, activations);
	pred->pre = malloc(sizeof(double)*num_y_vars);
	c->pred = pred;
}

void pred_neural_free(CL *c)
{
	PRED_NEURAL *pred = c->pred;
	neural_free(&pred->bpn);
	free(pred->pre);
	free(pred);
}

void pred_neural_copy(CL *to, CL *from)
{
	PRED_NEURAL *to_pred = to->pred;
	PRED_NEURAL *from_pred = from->pred;
	neural_copy(&to_pred->bpn, &from_pred->bpn);
}

void pred_neural_update(CL *c, double *y, double *x)
{
	PRED_NEURAL *pred = c->pred;
	neural_learn(&pred->bpn, y, x);
}

double *pred_neural_compute(CL *c, double *x)
{
	PRED_NEURAL *pred = c->pred;
	neural_propagate(&pred->bpn, x);
	for(int i = 0; i < num_y_vars; i++) {
		pred->pre[i] = neural_output(&pred->bpn, i);
	}
	return pred->pre;
}
                        
double pred_neural_pre(CL *c, int p)
{
	PRED_NEURAL *pred = c->pred;
	return pred->pre[p];
}
 
void pred_neural_print(CL *c)
{
	PRED_NEURAL *pred = c->pred;
	neural_print(&pred->bpn);
}  
