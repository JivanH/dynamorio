/* **********************************************************
 * Copyright (c) 2018 Google, Inc.  All rights reserved.
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
 * * Neither the name of Google, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "tools.h"
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>
#include <errno.h>
#include <stdlib.h>

#define ALT_STACK_SIZE (SIGSTKSZ * 4)

static void
signal_handler(int sig)
{
    print("Got signal %d\n", sig);
}

int
main(int argc, char *argv[])
{
    char *sp;
    int rc;
    INIT();

    /* Make an alternate stack that's not writable. */
    stack_t sigstack;
    sigstack.ss_sp = (char *)malloc(ALT_STACK_SIZE);
    sigstack.ss_size = ALT_STACK_SIZE;
    sigstack.ss_flags = SS_ONSTACK;
    rc = sigaltstack(&sigstack, NULL);
    ASSERT_NOERR(rc);
    protect_mem((void *)sigstack.ss_sp, ALT_STACK_SIZE, ALLOW_READ);

    /* Test checking for SA_ONSTACK: this one should be delivered to the main
     * stack and should work.
     */
    intercept_signal(SIGUSR1, (handler_3_t)signal_handler, false);
    print("Sending SIGUSR1\n");
    kill(getpid(), SIGUSR1);

    /* Now route to the alt stack, which is unwritable and thus should
     * crash with SIGSEGV, which we handle on the main stack and whose
     * resumption is the same post-kill point, letting us continue.
     */
    intercept_signal(SIGSEGV, (handler_3_t)signal_handler, false);
    intercept_signal(SIGUSR1, (handler_3_t)signal_handler, true);
    print("Sending SIGUSR1\n");
    kill(getpid(), SIGUSR1);

    print("All done\n");
    return 0;
}
