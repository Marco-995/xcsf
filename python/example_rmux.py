#!/usr/bin/python3
#
# Copyright (C) 2019--2020 Richard Preen <rpreen@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

################################################################################
# This example uses the reinforcement learning mechanism to construct and update
# match and action sets with classifiers composed of hyperrectangle conditions,
# linear least squares predictions, and integer actions to solve the real-mux.
################################################################################

import xcsf.xcsf as xcsf # Import XCSF
import numpy as np
from random import random
import matplotlib.pyplot as plt
from tqdm import tqdm
np.set_printoptions(suppress=True)

###########################
# Real-multiplexer problem
###########################

mux = 6 # total number of bits

# set the number of position bits
pos_bits = 1
while pos_bits + pow(2, pos_bits) <= mux:
    pos_bits += 1
pos_bits -= 1
print(str(mux)+" bits, "+str(pos_bits)+" position bits")

# current mux state
state = np.zeros(mux)

# random mux
def rmux_reset():
    for i in range(mux):
        state[i] = random()

# calculate mux answer
def rmux_answer():
    pos = pos_bits
    for i in range(pos_bits):
        if state[i] > 0.5:
            pos += pow(2, pos_bits - 1 - i)
    if state[pos] > 0.5:
        return 1
    return 0

# mux reward
def rmux_reward(action):
    if action == rmux_answer():
        return 1
    return 0

###################
# Initialise XCSF
###################

# initialise XCSF for single-step reinforcement learning
xcs = xcsf.XCS(mux, 2, False)

# override default.ini
xcs.OMP_NUM_THREADS = 8
xcs.POP_SIZE = 1000
xcs.PERF_TRIALS = 1000
xcs.EPS_0 = 0.01 # target error
xcs.COND_TYPE = 1 # hyperrectangles
xcs.PRED_TYPE = 1 # linear least squares
xcs.ACT_TYPE = 0 # integers
xcs.BETA = 0.2 # classifier parameter update rate
xcs.THETA_EA = 25 # EA frequency
xcs.ALPHA = 0.1 # accuracy offset
xcs.NU = 5 # accuracy slope
xcs.EA_SUBSUMPTION = True
xcs.SET_SUBSUMPTION = True
xcs.THETA_SUB = 100 # minimum experience of a subsumer

xcs.print_params()

#####################
# Execute experiment
#####################

n = 100 # 100,000 trials
trials = np.zeros(n)
psize = np.zeros(n)
msize = np.zeros(n)
performance = np.zeros(n)
error = np.zeros(n)
bar = tqdm(total=n) # progress bar

for i in range(n):
    for j in range(xcs.PERF_TRIALS):
        # explore trial
        rmux_reset()
        xcs.single_init_trial()
        action = xcs.single_decision(state, True)
        reward = rmux_reward(action)
        xcs.single_update(reward)
        xcs.single_end_trial()
        # exploit trial
        rmux_reset()
        xcs.single_init_trial()
        action = xcs.single_decision(state, False)
        reward = rmux_reward(action)
        performance[i] += reward
        error[i] += xcs.single_error(reward)
        xcs.single_end_trial()
    performance[i] /= float(xcs.PERF_TRIALS)
    error[i] /= float(xcs.PERF_TRIALS)
    trials[i] = xcs.time() # number of trials so far
    psize[i] = xcs.pop_size() # current population size
    msize[i] = xcs.msetsize() # avg match set size
    # update status
    status = ("trials=%d performance=%.5f error=%.5f psize=%d msize=%.1f"
        % (trials[i], performance[i], error[i], psize[i], msize[i]))
    bar.set_description(status)
    bar.refresh()
    bar.update(1)
bar.close()

#################################
# Plot XCSF learning performance
#################################

plt.figure(figsize=(10,6))
plt.plot(trials, performance, label='Performance')
plt.plot(trials, error, label='System error')
plt.grid(linestyle='dotted', linewidth=1)
plt.title('XCSF Training Performance', fontsize=14)
plt.xlabel('Trials', fontsize=12)
plt.xlim([0,n*xcs.PERF_TRIALS])
plt.legend()
plt.show()
