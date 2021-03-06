/*****************************************************************************\
 *  Copyright (c) 2014 Lawrence Livermore National Security, LLC.  Produced at
 *  the Lawrence Livermore National Laboratory (cf, AUTHORS, DISCLAIMER.LLNS).
 *  LLNL-CODE-658032 All rights reserved.
 *
 *  This file is part of the Flux resource manager framework.
 *  For details, see https://github.com/flux-framework.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the license, or (at your option)
 *  any later version.
 *
 *  Flux is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the terms and conditions of the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *  See also:  http://www.gnu.org/licenses/
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <czmq.h>
#include <flux/core.h>

#include "src/common/libutil/xzmalloc.h"
#include "src/common/libutil/log.h"
#include "src/common/libutil/shortjson.h"
#include "src/common/libutil/nodeset.h"

#include "hello.h"

/* After this many seconds, ignore topo-based hwm.
 * Override by setting hello.timeout broker attribute.
 */
static double default_reduction_timeout = 10.;

struct hello_struct {
    flux_t h;
    attr_t *attrs;
    uint32_t rank;
    uint32_t size;
    uint32_t count;

    double start;

    hello_cb_f cb;
    void *cb_arg;

    flux_reduce_t *reduce;
};

static void join_request (flux_t h, flux_msg_handler_t *w,
                          const flux_msg_t *msg, void *arg);

static void r_reduce (flux_reduce_t *r, int batch, void *arg);
static void r_sink (flux_reduce_t *r, int batch, void *arg);
static void r_forward (flux_reduce_t *r, int batch, void *arg);
static int r_itemweight (void *item);


struct flux_reduce_ops reduce_ops = {
    .destroy = NULL,
    .reduce = r_reduce,
    .sink = r_sink,
    .forward = r_forward,
    .itemweight = r_itemweight,
};

hello_t *hello_create (void)
{
    hello_t *hello = xzmalloc (sizeof (*hello));
    hello->size = 1;
    return hello;
}

void hello_destroy (hello_t *hello)
{
    if (hello) {
        flux_reduce_destroy (hello->reduce);
        free (hello);
    }
}

static int hwm_from_topology (attr_t *attrs)
{
    const char *s;
    if (attr_get (attrs, "tbon.descendants", &s, NULL) < 0) {
        log_err ("hello: reading tbon.descendants attribute");
        return 1;
    }
    return strtoul (s, NULL, 10) + 1;
}

int hello_register_attrs (hello_t *hello, attr_t *attrs)
{
    const char *s;
    double timeout = default_reduction_timeout;
    int hwm = hwm_from_topology (attrs);
    char num[32];

    hello->attrs = attrs;
    if (attr_get (attrs, "hello.timeout", &s, NULL) == 0) {
        timeout = strtod (s, NULL);
        if (attr_delete (attrs, "hello.timeout", true) < 0)
            return -1;
    }
    snprintf (num, sizeof (num), "%.3f", timeout);
    if (attr_add (attrs, "hello.timeout", num, FLUX_ATTRFLAG_IMMUTABLE) < 0)
        return -1;
    if (attr_get (attrs, "hello.hwm", &s, NULL) == 0) {
        hwm = strtoul (s, NULL, 10);
        if (attr_delete (attrs, "hello.hwm", true) < 0)
            return -1;
    }
    snprintf (num, sizeof (num), "%d", hwm);
    if (attr_add (attrs, "hello.hwm", num, FLUX_ATTRFLAG_IMMUTABLE) < 0)
        return -1;
    return 0;
}

static struct flux_msg_handler_spec handlers[] = {
    { FLUX_MSGTYPE_REQUEST, "hello.join",     join_request },
    FLUX_MSGHANDLER_TABLE_END,
};

void hello_set_flux (hello_t *hello, flux_t h)
{
    hello->h = h;
}

double hello_get_time (hello_t *hello)
{
    if (hello->start == 0. || hello->h == NULL)
        return 0.;
    return flux_reactor_now (flux_get_reactor (hello->h)) - hello->start;
}

int hello_get_count (hello_t *hello)
{
    return hello->count;
}

void hello_set_callback (hello_t *hello, hello_cb_f cb, void *arg)
{
    hello->cb = cb;
    hello->cb_arg = arg;
}

bool hello_complete (hello_t *hello)
{
    return (hello->size == hello->count);
}

int hello_start (hello_t *hello)
{
    int rc = -1;
    int flags = 0;
    int hwm = 1;
    double timeout = 0.;
    const char *s;

    if (flux_get_rank (hello->h, &hello->rank) < 0
                        || flux_get_size (hello->h, &hello->size) < 0) {
        log_err ("hello: error getting rank/size");
        goto done;
    }
    if (flux_msg_handler_addvec (hello->h, handlers, hello) < 0) {
        log_err ("hello: adding message handlers");
        goto done;
    }
    if (hello->attrs) {
        if (attr_get (hello->attrs, "hello.hwm", &s, NULL) < 0) {
            log_err ("hello: reading hello.hwm attribute");
            goto done;
        }
        hwm = strtoul (s, NULL, 10);
        if (attr_get (hello->attrs, "hello.timeout", &s, NULL) < 0) {
            log_err ("hello: reading hello.timeout attribute");
            goto done;
        }
        timeout = strtod (s, NULL);
    }
    if (timeout > 0.)
        flags |= FLUX_REDUCE_TIMEDFLUSH;
    if (hwm > 0)
        flags |= FLUX_REDUCE_HWMFLUSH;
    if (!(hello->reduce = flux_reduce_create (hello->h, reduce_ops,
                                              timeout, hello, flags))) {
        log_err ("hello: creating reduction handle");
        goto done;
    }
    if (flux_reduce_opt_set (hello->reduce, FLUX_REDUCE_OPT_HWM,
                             &hwm, sizeof (hwm)) < 0) {
        log_err ("hello: setting FLUX_REDUCE_OPT_HWM");
        goto done;
    }

    flux_reactor_t *r = flux_get_reactor (hello->h);
    flux_reactor_now_update (r);
    hello->start = flux_reactor_now (r);
    if (flux_reduce_append (hello->reduce, (void *)(uintptr_t)1, 0) < 0)
        goto done;
    rc = 0;
done:
    return rc;
}

/* handle a message sent from downstream via downstream's r_forward op.
 */
static void join_request (flux_t h, flux_msg_handler_t *w,
                          const flux_msg_t *msg, void *arg)
{
    hello_t *hello = arg;
    const char *json_str;
    int count, batch;
    JSON in = NULL;

    if (flux_request_decode (msg, NULL, &json_str) < 0)
        log_err_exit ("hello: flux_request_decode");
    if (!(in = Jfromstr (json_str)) || !Jget_int (in, "count", &count)
                                    || !Jget_int (in, "batch", &batch)
                                    || batch != 0 || count <= 0)
        log_msg_exit ("hello: error decoding join request");
    if (flux_reduce_append (hello->reduce, (void *)(uintptr_t)count, batch) < 0)
        log_err_exit ("hello: flux_reduce_append");
}

/* Reduction ops
 * N.B. since we are reducing integers, we cheat and avoid memory
 * allocation by stuffing the int inside the pointer value.
 */

/* Pop one or more counts, push their sum
 */
static void r_reduce (flux_reduce_t *r, int batch, void *arg)
{
    int i, count = 0;

    assert (batch == 0);

    while ((i = (uintptr_t)flux_reduce_pop (r)) > 0)
        count += i;
    if (count > 0 && flux_reduce_push (r, (void *)(uintptr_t)count) < 0)
        log_err_exit ("hello: flux_reduce_push");
    /* Invariant for r_sink and r_forward:
     * after reduce, handle contains exactly one item.
     */
}

/* (called on rank 0 only) Pop exactly one count, update global count,
 * call the registered callback.
 * This may be called once the total hwm is reached on rank 0,
 * or after the timeout, as new messages arrive (after r_reduce).
 */
static void r_sink (flux_reduce_t *r, int batch, void *arg)
{
    hello_t *hello = arg;
    int count = (uintptr_t)flux_reduce_pop (r);

    assert (batch == 0);
    assert (count > 0);

    hello->count += count;
    if (hello->cb)
        hello->cb (hello, hello->cb_arg);
}

/* (called on rank > 0 only) Pop exactly one count, forward upstream.
 * This may be called once the hwm is reached on this rank (based on topo),
 * or after the timeout, as new messages arrive (after r_reduce).
 */
static void r_forward (flux_reduce_t *r, int batch, void *arg)
{
    flux_rpc_t *rpc;
    hello_t *hello = arg;
    int count = (uintptr_t)flux_reduce_pop (r);
    JSON in = Jnew ();

    assert (batch == 0);
    assert (count > 0);

    Jadd_int (in, "count", count);
    Jadd_int (in, "batch", batch);
    if (!(rpc = flux_rpc (hello->h, "hello.join", Jtostr (in),
                          FLUX_NODEID_UPSTREAM, FLUX_RPC_NORESPONSE)))
        log_err_exit ("hello: flux_rpc");
    flux_rpc_destroy (rpc);
    Jput (in);
}

/* How many original items does this item represent after reduction?
 * In this simple case it is just the value of the item (the count).
 */
static int r_itemweight (void *item)
{
    return (uintptr_t)item;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
