/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* GNU pth threads interface
   http://www.gnu.org/software/pth
   2000-05-03 Andy Dustman <andy@dustman.net>

   Adapted from Posix threads interface 
   12 May 1997 -- david arnold <davida@pobox.com>
 */

#include <stdlib.h>
#include <string.h>
#include <pth.h>

/* A pth mutex isn't sufficient to model the Python lock type
 * because pth mutexes can be acquired multiple times by the
 * same thread.
 *
 * The pth_lock struct implements a Python lock as a "locked?" bit
 * and a <condition, mutex> pair.  In general, if the bit can be acquired
 * instantly, it is, else the pair is used to block the thread until the
 * bit is cleared.
 */

typedef struct {
	char             locked; /* 0=unlocked, 1=locked */
	/* a <cond, mutex> pair to handle an acquire of a locked lock */
	pth_cond_t   lock_released;
	pth_mutex_t  mut;
} pth_lock;

#define CHECK_STATUS(name)  if (status == -1) { printf("%d ", status); perror(name); error = 1; }

/*
 * Initialization.
 */

static void PyThread__init_thread _P0()
{
	pth_init();
}

/*
 * Thread support.
 */


int PyThread_start_new_thread _P2(func, void (*func) _P((void *)), arg, void *arg)
{
	pth_t th;
	int success;
	dprintf(("PyThread_start_new_thread called\n"));
	if (!initialized)
		PyThread_init_thread();

	th = pth_spawn(PTH_ATTR_DEFAULT,
				 (void* (*)_P((void *)))func,
				 (void *)arg
				 );

	return th == NULL ? 0 : 1;
}

long PyThread_get_thread_ident _P0()
{
	volatile pth_t threadid;
	if (!initialized)
		PyThread_init_thread();
	/* Jump through some hoops for Alpha OSF/1 */
	threadid = pth_self();
	return (long) *(long *) &threadid;
}

static void do_PyThread_exit_thread _P1(no_cleanup, int no_cleanup)
{
	dprintf(("PyThread_exit_thread called\n"));
	if (!initialized) {
		if (no_cleanup)
			_exit(0);
		else
			exit(0);
	}
}

void PyThread_exit_thread _P0()
{
	do_PyThread_exit_thread(0);
}

void PyThread__exit_thread _P0()
{
	do_PyThread_exit_thread(1);
}

#ifndef NO_EXIT_PROG
static void do_PyThread_exit_prog _P2(status, int status, no_cleanup, int no_cleanup)
{
	dprintf(("PyThread_exit_prog(%d) called\n", status));
	if (!initialized)
		if (no_cleanup)
			_exit(status);
		else
			exit(status);
}

void PyThread_exit_prog _P1(status, int status)
{
	do_PyThread_exit_prog(status, 0);
}

void PyThread__exit_prog _P1(status, int status)
{
	do_PyThread_exit_prog(status, 1);
}
#endif /* NO_EXIT_PROG */

/*
 * Lock support.
 */
PyThread_type_lock PyThread_allocate_lock _P0()
{
	pth_lock *lock;
	int status, error = 0;

	dprintf(("PyThread_allocate_lock called\n"));
	if (!initialized)
		PyThread_init_thread();

	lock = (pth_lock *) malloc(sizeof(pth_lock));
        memset((void *)lock, '\0', sizeof(pth_lock));
	if (lock) {
		lock->locked = 0;
		status = pth_mutex_init(&lock->mut);
		CHECK_STATUS("pth_mutex_init");
		status = pth_cond_init(&lock->lock_released);
		CHECK_STATUS("pth_cond_init");
		if (error) {
			free((void *)lock);
			lock = NULL;
		}
	}
	dprintf(("PyThread_allocate_lock() -> %lx\n", (long)lock));
	return (PyThread_type_lock) lock;
}

void PyThread_free_lock _P1(lock, PyThread_type_lock lock)
{
	pth_lock *thelock = (pth_lock *)lock;
	int status, error = 0;

	dprintf(("PyThread_free_lock(%lx) called\n", (long)lock));

	free((void *)thelock);
}

int PyThread_acquire_lock _P2(lock, PyThread_type_lock lock, waitflag, int waitflag)
{
	int success;
	pth_lock *thelock = (pth_lock *)lock;
	int status, error = 0;

	dprintf(("PyThread_acquire_lock(%lx, %d) called\n", (long)lock, waitflag));

	status = pth_mutex_acquire(&thelock->mut, !waitflag, NULL);
	CHECK_STATUS("pth_mutex_acquire[1]");
	success = thelock->locked == 0;
        if (success) thelock->locked = 1;
        status = pth_mutex_release( &thelock->mut );
        CHECK_STATUS("pth_mutex_release[1]");

        if ( !success && waitflag ) {
                /* continue trying until we get the lock */

                /* mut must be locked by me -- part of the condition
                 * protocol */
                status = pth_mutex_acquire( &thelock->mut, !waitflag, NULL );
                CHECK_STATUS("pth_mutex_acquire[2]");
                while ( thelock->locked ) {
                        status = pth_cond_await(&thelock->lock_released,
                                                &thelock->mut, NULL);
                        CHECK_STATUS("pth_cond_await");
                }
                thelock->locked = 1;
                status = pth_mutex_release( &thelock->mut );
                CHECK_STATUS("pth_mutex_release[2]");
                success = 1;
        }
        if (error) success = 0;
        dprintf(("PyThread_acquire_lock(%lx, %d) -> %d\n", (long)lock, waitflag, success));
	return success;
}

void PyThread_release_lock _P1(lock, PyThread_type_lock lock)
{
        pth_lock *thelock = (pth_lock *)lock;
        int status, error = 0;

        dprintf(("PyThread_release_lock(%lx) called\n", (long)lock));

        status = pth_mutex_acquire( &thelock->mut, 0, NULL );
        CHECK_STATUS("pth_mutex_acquire[3]");

        thelock->locked = 0;

        status = pth_mutex_release( &thelock->mut );
        CHECK_STATUS("pth_mutex_release[3]");

        /* wake up someone (anyone, if any) waiting on the lock */
        status = pth_cond_notify( &thelock->lock_released, 0 );
        CHECK_STATUS("pth_cond_notify");
}

/*
 * Semaphore support.
 */

struct semaphore {
	pth_mutex_t mutex;
	pth_cond_t cond;
	int value;
};

PyThread_type_sema PyThread_allocate_sema _P1(value, int value)
{
	struct semaphore *sema;
	int status, error = 0;

	dprintf(("PyThread_allocate_sema called\n"));
	if (!initialized)
		PyThread_init_thread();

	sema = (struct semaphore *) malloc(sizeof(struct semaphore));
	if (sema != NULL) {
		sema->value = value;
		status = pth_mutex_init(&sema->mutex);
		CHECK_STATUS("pth_mutex_init");
		status = pth_cond_init(&sema->cond);
		CHECK_STATUS("pth_mutex_init");
		if (error) {
			free((void *) sema);
			sema = NULL;
		}
	}
	dprintf(("PyThread_allocate_sema() -> %lx\n", (long) sema));
	return (PyThread_type_sema) sema;
}

void PyThread_free_sema _P1(sema, PyThread_type_sema sema)
{
	int status, error = 0;
	struct semaphore *thesema = (struct semaphore *) sema;

	dprintf(("PyThread_free_sema(%lx) called\n", (long) sema));
	free((void *) thesema);
}

int PyThread_down_sema _P2(sema, PyThread_type_sema sema, waitflag, int waitflag)
{
	int status, error = 0, success;
	struct semaphore *thesema = (struct semaphore *) sema;

	dprintf(("PyThread_down_sema(%lx, %d) called\n", (long) sema, waitflag));
	status = pth_mutex_acquire(&thesema->mutex, !waitflag, NULL);
	CHECK_STATUS("pth_mutex_acquire");
	if (waitflag) {
		while (!error && thesema->value <= 0) {
			status = pth_cond_await(&thesema->cond,
						&thesema->mutex, NULL);
			CHECK_STATUS("pth_cond_await");
		}
	}
	if (error)
		success = 0;
	else if (thesema->value > 0) {
		thesema->value--;
		success = 1;
	}
	else
		success = 0;
	status = pth_mutex_release(&thesema->mutex);
	CHECK_STATUS("pth_mutex_release");
	dprintf(("PyThread_down_sema(%lx) return\n", (long) sema));
	return success;
}

void PyThread_up_sema _P1(sema, PyThread_type_sema sema)
{
	int status, error = 0;
	struct semaphore *thesema = (struct semaphore *) sema;

	dprintf(("PyThread_up_sema(%lx)\n", (long) sema));
	status = pth_mutex_acquire(&thesema->mutex, 0, NULL);
	CHECK_STATUS("pth_mutex_acquire");
	thesema->value++;
	status = pth_cond_notify(&thesema->cond, 1);
	CHECK_STATUS("pth_cond_notify");
	status = pth_mutex_release(&thesema->mutex);
	CHECK_STATUS("pth_mutex_release");
}
