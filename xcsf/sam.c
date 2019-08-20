/*
 * Copyright (C) 2015--2019 Richard Preen <rpreen@gmail.com>
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
 * The self-adaptive mutation module.
 *
 * Initialises the classifier mutation rates and performs self-adaptation using
 * a normal distribution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "data_structures.h"
#include "random.h"

void sam_init(XCSF *xcsf, double **mu)
{
	if(xcsf->NUM_SAM > 0) {
		*mu = malloc(sizeof(double) * xcsf->NUM_SAM);
		for(int i = 0; i < xcsf->NUM_SAM; i++) {
			(*mu)[i] = rand_uniform(0,1);
		}
	}
}

void sam_copy(XCSF *xcsf, double *to, double *from)
{
	memcpy(to, from, sizeof(double) * xcsf->NUM_SAM);
}

void sam_free(XCSF *xcsf, double *mu)
{
	if(xcsf->NUM_SAM > 0) {
		free(mu);
	}
}

void sam_adapt(XCSF *xcsf, double *mu)
{
	// adapt rates
	for(int i = 0; i < xcsf->NUM_SAM; i++) {
		mu[i] *= exp(rand_normal());
		if(mu[i] < xcsf->muEPS_0) {
			mu[i] = xcsf->muEPS_0;
		}
		else if(mu[i] > 1.0) {
			mu[i] = 1.0;
		}
	}
	// apply rates
	if(xcsf->NUM_SAM > 0) {
		xcsf->P_MUTATION = mu[0];
		if(xcsf->NUM_SAM > 1) {
			xcsf->S_MUTATION = mu[1];
			if(xcsf->NUM_SAM > 2) {
				xcsf->P_FUNC_MUTATION = mu[2];
			}
		}
	} 
}

void sam_print(XCSF *xcsf, double *mu)
{
	printf("mu: \n");
	for(int i = 0; i < xcsf->NUM_SAM; i++) {
		printf("%f, ", mu[i]);
	}
	printf("\n");
}
