#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "pmi.h"

#ifndef PMI_FALSE
#define PMI_FALSE 0
#endif
#ifndef PMI_TRUE
#define PMI_TRUE 1
#endif

static void errx (char *msg, int rc)
{
    fprintf (stderr, "%s: %s\n", msg,
        rc == PMI_SUCCESS ? "success" :
        rc == PMI_FAIL ? "fail" :
        rc == PMI_ERR_INIT ? "err_init" :
        rc == PMI_ERR_INVALID_ARG ? "err_invalid_arg" :
        rc == PMI_ERR_INVALID_KEY ? "err_invalid_key" :
        rc == PMI_ERR_INVALID_KEY_LENGTH ? "err_invalid_key_length" :
        rc == PMI_ERR_INVALID_VAL ? "err_invalid_val" :
        rc == PMI_ERR_INVALID_VAL_LENGTH ? "err_invalid_val_length" :
        rc == PMI_ERR_INVALID_NUM_ARGS ? "invalid_num_args" :
        rc == PMI_ERR_INVALID_ARGS ? "invalid_args" :
        rc == PMI_ERR_INVALID_NUM_PARSED? "invalid_num_parsed" :
        rc == PMI_ERR_INVALID_KEYVALP ? "invalid_keyvalp" :
        rc == PMI_ERR_INVALID_SIZE ? "invalid_size" : "UNKNOWN ERROR");
    exit (1);
}

int main (int argc, char **argv)
{
    int rc;
    int spawned = -1;
    int initialized = -1;

    rc = PMI_Initialized (&initialized);
    if (rc != PMI_SUCCESS)
        errx ("PMI_Initialized", rc);
    assert (initialized == PMI_FALSE);

    rc = PMI_Init (&spawned);
    if (rc != PMI_SUCCESS)
        errx ("PMI_Init", rc);

    rc = PMI_Initialized (&initialized);
    if (rc != PMI_SUCCESS)
        errx ("PMI_Initialized", rc);
    assert (initialized == PMI_TRUE);

    rc = PMI_Finalize ();
    if (rc != PMI_SUCCESS)
        errx ("PMI_Finalize", rc);

    rc = PMI_Initialized (&initialized);
    if (rc != PMI_SUCCESS)
        errx ("PMI_Initialized", rc);
    assert (initialized == PMI_FALSE);
    
    exit (0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */