/* **********************************************************
 * Copyright (c) 2011-2019 Google, Inc.  All rights reserved.
 * Copyright (c) 2003-2008 VMware, Inc.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of VMware, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/* This test is a modified version of api.detach_spawn. It adds start/stop re-attach to
 * api.detach_spawn. It increases the number of spawned parent and child threads to 25
 * each, spawning a total number of 25 + 25 * 25 = 650 threads at a time.
 */

#include <stdio.h>
#include <math.h>
#include "configure.h"
#include "dr_api.h"
#include "tools.h"
#include "thread.h"
#include "condvar.h"

#define VERBOSE 0
#define NUM_PARENT_THREADS 25
#define NUM_CHILD_THREADS 25

static void *parent_ready;
static volatile bool parent_exit = false;

#if VERBOSE
#    define VPRINT(...) print(__VA_ARGS__)
#else
#    define VPRINT(...) /* nothing */
#endif

THREAD_FUNC_RETURN_TYPE
child_func(void *arg)
{
    return THREAD_FUNC_RETURN_ZERO;
}

THREAD_FUNC_RETURN_TYPE
parent_func(void *arg)
{
    signal_cond_var(parent_ready);
    int loops = 0;
    while (!parent_exit) {
        if (loops++ < 50) {
            int i;
            thread_t threads[NUM_CHILD_THREADS];
            for (i = 0; i < NUM_CHILD_THREADS; ++i) {
                threads[i] = create_thread(child_func, NULL);
            }
            for (i = 0; i < NUM_CHILD_THREADS; ++i)
                join_thread(threads[i]);
        }
    }
    return THREAD_FUNC_RETURN_ZERO;
}

int
main(void)
{
    parent_ready = create_cond_var();
    int i;
    thread_t threads[NUM_PARENT_THREADS];

    print("Starting threads\n");

    for (i = 0; i < NUM_PARENT_THREADS; ++i)
        threads[i] = create_thread(parent_func, NULL);

    /* We setup and start at once to avoid process memory changing much between
     * the two.
     */
    dr_app_setup_and_start();
    wait_cond_var(parent_ready);
    thread_sleep(50);

    if (!dr_app_running_under_dynamorio()) {
        print("ERROR: should be running under DynamoRIO before calling "
              "dr_app_stop_and_cleanup()\n");
    }

    print("Running under DynamoRIO\n");

    dr_app_stop_and_cleanup();

    thread_sleep(50);

    if (dr_app_running_under_dynamorio()) {
        print("ERROR: should not be running under DynamoRIO before calling "
              "dr_app_stop()\n");
    }

    print("Not running under DynamoRIO\n");

    dr_app_setup_and_start();

    thread_sleep(50);

    if (!dr_app_running_under_dynamorio()) {
        print("ERROR: should be running under DynamoRIO before calling "
              "dr_app_stop_and_cleanup()\n");
    }

    print("Running under DynamoRIO\n");

    dr_app_stop_and_cleanup();

    print("Not running under DynamoRIO, exiting\n");

    parent_exit = true;
    for (i = 0; i < NUM_PARENT_THREADS; ++i)
        join_thread(threads[i]);
    destroy_cond_var(parent_ready);

    print("all done\n");
    return 0;
}
