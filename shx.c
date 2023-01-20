/*
 * Read ESRI shapefiles
 *
 * Copyright (C) 2023 Andreas Vögele
 *
 * This library is free software; you can redistribute it and/or modify it
 * under either the terms of the ISC License or the same terms as Perl.
 */

/* SPDX-License-Identifier: ISC OR Artistic-1.0-Perl OR GPL-1.0-or-later */

#include "shx.h"
#include "convert.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

shx_file_t *
shx_file(shx_file_t *fh, FILE *fp, void *user_data)
{
    return shp_file(fh, fp, user_data);
}

void
shx_error(shx_file_t *fh, const char *format, ...)
{
    va_list ap;

    assert(fh != NULL);
    assert(format != NULL);

    va_start(ap, format);
    vsnprintf(fh->error, sizeof(fh->error), format, ap); /* NOLINT */
    va_end(ap);
}

int
shx_read_header(shx_file_t *fh, shx_header_t **pheader)
{
    return shp_read_header(fh, pheader);
}

static int
read_record(shx_file_t *fh, shx_record_t *precord)
{
    int rc = -1;
    char buf[8];
    size_t nr;

    nr = fread(buf, 1, 8, fh->fp);
    fh->num_bytes += nr;
    if (ferror(fh->fp)) {
        shp_error(fh, "Cannot read index record");
        goto cleanup;
    }
    if (feof(fh->fp)) {
        /* Reached end of file. */
        rc = 0;
        goto cleanup;
    }
    if (nr != 8) {
        shp_error(fh, "Expected index record of %zu bytes, got %zu",
                  (size_t) 8, nr);
        errno = EINVAL;
        goto cleanup;
    }

    precord->file_offset = shp_be32_to_int32(&buf[0]);
    precord->content_length = shp_be32_to_int32(&buf[4]);

    rc = 1;

cleanup:

    return rc;
}

int
shx_read_record(shx_file_t *fh, shx_record_t **precord)
{
    int rc = -1;
    shx_record_t *record = NULL;

    assert(fh != NULL);
    assert(fh->fp != NULL);
    assert(precord != NULL);

    record = (shx_record_t *) malloc(sizeof(*record));
    if (record == NULL) {
        shx_error(fh, "Cannot allocate %zu bytes", sizeof(*record));
        goto cleanup;
    }
    rc = read_record(fh, record);
    if (rc <= 0) {
        free(record);
        record = NULL;
    }

cleanup:

    *precord = record;

    return rc;
}

int
shx_read(shx_file_t *fh, shx_header_callback_t handle_header,
         shx_record_callback_t handle_record)
{
    int rc = -1, rc2;
    shx_header_t *header = NULL;
    shx_record_t record;

    assert(fh != NULL);
    assert(fh->fp != NULL);
    assert(handle_header != NULL);
    assert(handle_record != NULL);

    rc2 = shx_read_header(fh, &header);
    if (rc2 == 0) {
        /* Reached end of file. */
        rc = 0;
    }
    if (rc2 <= 0) {
        goto cleanup;
    }

    assert(header != NULL);

    rc2 = (*handle_header)(fh, header);
    if (rc2 == 0) {
        /* Stop processing. */
        rc = 0;
    }
    if (rc2 <= 0) {
        goto cleanup;
    }

    for (;;) {
        rc2 = read_record(fh, &record);
        if (rc2 == 0) {
            /* Reached end of file. */
            rc = 0;
        }
        if (rc2 <= 0) {
            goto cleanup;
        }

        rc2 = (*handle_record)(fh, header, &record);
        if (rc2 == 0) {
            /* Stop processing. */
            rc = 0;
        }
        if (rc2 <= 0) {
            goto cleanup;
        }
    }

cleanup:

    free(header);

    return rc;
}
