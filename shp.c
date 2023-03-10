/*
 * Read ESRI shapefiles
 *
 * Copyright (C) 2023 Andreas Vögele
 *
 * This library is free software; you can redistribute it and/or modify it
 * under either the terms of the ISC License or the same terms as Perl.
 */

/* SPDX-License-Identifier: ISC OR Artistic-1.0-Perl OR GPL-1.0-or-later */

#include "shp.h"
#include "convert.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef SHP_MIN_BUF_SIZE
#define SHP_MIN_BUF_SIZE 26214400
#endif

shp_file_t *
shp_init_file(shp_file_t *fh, FILE *fp, void *user_data)
{
    assert(fh != NULL);
    assert(fp != NULL);

    fh->fp = fp;
    fh->user_data = user_data;
    fh->num_bytes = 0;
    fh->error[0] = '\0';

    return fh;
}

void
shp_set_error(shp_file_t *fh, const char *format, ...)
{
    va_list ap;

    assert(fh != NULL);
    assert(format != NULL);

    va_start(ap, format);
    vsnprintf(fh->error, sizeof(fh->error), format, ap); /* NOLINT */
    va_end(ap);
}

int
shp_read_header(shp_file_t *fh, shp_header_t *header)
{
    int rc = -1;
    char buf[100];
    long file_code;
    size_t nr;

    assert(fh != NULL);
    assert(fh->fp != NULL);
    assert(header != NULL);

    nr = fread(buf, 1, 100, fh->fp);
    fh->num_bytes += nr;
    if (ferror(fh->fp)) {
        shp_set_error(fh, "Cannot read file header");
        goto cleanup;
    }
    if (nr != 100) {
        shp_set_error(fh, "Expected file header of %zu bytes, got %zu",
                      (size_t) 100, nr);
        errno = EINVAL;
        goto cleanup;
    }

    file_code = shp_be32_to_int32(&buf[0]);
    if (file_code != 9994) {
        shp_set_error(fh, "Expected file code 9994, got %ld", file_code);
        errno = EINVAL;
        goto cleanup;
    }

    header->file_code = file_code;
    header->unused[0] = shp_be32_to_int32(&buf[4]);
    header->unused[1] = shp_be32_to_int32(&buf[8]);
    header->unused[2] = shp_be32_to_int32(&buf[12]);
    header->unused[3] = shp_be32_to_int32(&buf[16]);
    header->unused[4] = shp_be32_to_int32(&buf[20]);
    header->file_size = 2 * (size_t) shp_be32_to_uint32(&buf[24]);
    header->version = shp_le32_to_int32(&buf[28]);
    header->shape_type = (shp_shpt_t) shp_le32_to_int32(&buf[32]);
    header->x_min = shp_le64_to_double(&buf[36]);
    header->y_min = shp_le64_to_double(&buf[44]);
    header->x_max = shp_le64_to_double(&buf[52]);
    header->y_max = shp_le64_to_double(&buf[60]);
    header->z_min = shp_le64_to_double(&buf[68]);
    header->z_max = shp_le64_to_double(&buf[76]);
    header->m_min = shp_le64_to_double(&buf[84]);
    header->m_max = shp_le64_to_double(&buf[92]);

    if (feof(fh->fp)) {
        rc = 0;
        goto cleanup;
    }

    rc = 1;

cleanup:

    return rc;
}

static int
get_point(shp_file_t *fh, const char *buf, shp_record_t *record)
{
    int rc = -1;
    shp_point_t *point = &record->shape.point;
    size_t record_size, expected_size = 20;

    record_size = record->record_size;
    if (record_size != expected_size) {
        shp_set_error(fh,
                      "Expected record of %zu bytes, got %zu in record %zu",
                      expected_size, record_size, record->record_number);
        errno = EINVAL;
        goto cleanup;
    }

    point->x = shp_le64_to_double(&buf[4]);
    point->y = shp_le64_to_double(&buf[12]);

    rc = 1;

cleanup:

    return rc;
}

static int
get_pointm(shp_file_t *fh, const char *buf, shp_record_t *record)
{
    int rc = -1;
    shp_pointm_t *point = &record->shape.pointm;
    size_t record_size, expected_size = 28;

    record_size = record->record_size;
    if (record_size != expected_size) {
        shp_set_error(fh,
                      "Expected record of %zu bytes, got %zu in record %zu",
                      expected_size, record_size, record->record_number);
        errno = EINVAL;
        goto cleanup;
    }

    point->x = shp_le64_to_double(&buf[4]);
    point->y = shp_le64_to_double(&buf[12]);
    point->m = shp_le64_to_double(&buf[20]);

    rc = 1;

cleanup:

    return rc;
}

static int
get_multipoint(shp_file_t *fh, const char *buf, shp_record_t *record)
{
    int rc = -1;
    shp_multipoint_t *multipoint = &record->shape.multipoint;
    size_t record_size, points_size, expected_size;

    record_size = record->record_size;
    if (record_size < 40) {
        shp_set_error(fh, "Record size %zu is too small in record %zu",
                      record_size, record->record_number);
        errno = EINVAL;
        goto cleanup;
    }

    multipoint->box.x_min = shp_le64_to_double(&buf[4]);
    multipoint->box.y_min = shp_le64_to_double(&buf[12]);
    multipoint->box.x_max = shp_le64_to_double(&buf[20]);
    multipoint->box.y_max = shp_le64_to_double(&buf[28]);
    multipoint->num_points = shp_le32_to_uint32(&buf[36]);

    points_size = 16 * multipoint->num_points;

    expected_size = 40 + points_size;
    if (record_size != expected_size) {
        shp_set_error(fh,
                      "Expected record of %zu bytes, got %zu in record %zu",
                      expected_size, record_size, record->record_number);
        errno = EINVAL;
        goto cleanup;
    }

    multipoint->_points = &buf[40];

    rc = 1;

cleanup:

    return rc;
}

static int
get_multipointm(shp_file_t *fh, const char *buf, shp_record_t *record)
{
    int rc = -1;
    shp_multipointm_t *multipointm = &record->shape.multipointm;
    size_t record_size, points_size, measures_size, expected_size;

    record_size = record->record_size;
    if (record_size < 56) {
        shp_set_error(fh, "Record size %zu is too small in record %zu",
                      record_size, record->record_number);
        errno = EINVAL;
        goto cleanup;
    }

    multipointm->box.x_min = shp_le64_to_double(&buf[4]);
    multipointm->box.y_min = shp_le64_to_double(&buf[12]);
    multipointm->box.x_max = shp_le64_to_double(&buf[20]);
    multipointm->box.y_max = shp_le64_to_double(&buf[28]);
    multipointm->num_points = shp_le32_to_uint32(&buf[36]);

    points_size = 16 * multipointm->num_points;
    measures_size = 8 * multipointm->num_points;

    expected_size = 56 + points_size + measures_size;
    if (record_size != expected_size) {
        shp_set_error(fh,
                      "Expected record of %zu bytes, got %zu in record %zu",
                      expected_size, record_size, record->record_number);
        errno = EINVAL;
        goto cleanup;
    }

    multipointm->_points = &buf[40];
    multipointm->measure_range.min =
        shp_le64_to_double(&buf[40 + points_size]);
    multipointm->measure_range.max =
        shp_le64_to_double(&buf[48 + points_size]);
    multipointm->_measures = &buf[56 + points_size];

    rc = 1;

cleanup:

    return rc;
}

static int
get_polygon(shp_file_t *fh, const char *buf, shp_record_t *record)
{
    int rc = -1;
    shp_polygon_t *polygon = &record->shape.polygon;
    size_t record_size, parts_size, points_size, expected_size;

    record_size = record->record_size;
    if (record_size < 44) {
        shp_set_error(fh, "Record size %zu is too small in record %zu",
                      record_size, record->record_number);
        errno = EINVAL;
        goto cleanup;
    }

    polygon->box.x_min = shp_le64_to_double(&buf[4]);
    polygon->box.y_min = shp_le64_to_double(&buf[12]);
    polygon->box.x_max = shp_le64_to_double(&buf[20]);
    polygon->box.y_max = shp_le64_to_double(&buf[28]);
    polygon->num_parts = shp_le32_to_uint32(&buf[36]);
    polygon->num_points = shp_le32_to_uint32(&buf[40]);

    parts_size = 4 * polygon->num_parts;
    points_size = 16 * polygon->num_points;

    expected_size = 44 + parts_size + points_size;
    if (record_size != expected_size) {
        shp_set_error(fh,
                      "Expected record of %zu bytes, got %zu in record %zu",
                      expected_size, record_size, record->record_number);
        errno = EINVAL;
        goto cleanup;
    }

    polygon->_parts = &buf[44];
    polygon->_points = &buf[44 + parts_size];

    rc = 1;

cleanup:

    return rc;
}

static int
get_polyline(shp_file_t *fh, const char *buf, shp_record_t *record)
{
    int rc = -1;
    shp_polyline_t *polyline = &record->shape.polyline;
    size_t record_size, parts_size, points_size, expected_size;

    record_size = record->record_size;
    if (record_size < 44) {
        shp_set_error(fh, "Record size %zu is too small in record %zu",
                      record_size, record->record_number);
        errno = EINVAL;
        goto cleanup;
    }

    polyline->box.x_min = shp_le64_to_double(&buf[4]);
    polyline->box.y_min = shp_le64_to_double(&buf[12]);
    polyline->box.x_max = shp_le64_to_double(&buf[20]);
    polyline->box.y_max = shp_le64_to_double(&buf[28]);
    polyline->num_parts = shp_le32_to_uint32(&buf[36]);
    polyline->num_points = shp_le32_to_uint32(&buf[40]);

    parts_size = 4 * polyline->num_parts;
    points_size = 16 * polyline->num_points;

    expected_size = 44 + parts_size + points_size;
    if (record_size != expected_size) {
        shp_set_error(fh,
                      "Expected record of %zu bytes, got %zu in record %zu",
                      expected_size, record_size, record->record_number);
        errno = EINVAL;
        goto cleanup;
    }

    polyline->_parts = &buf[44];
    polyline->_points = &buf[44 + parts_size];

    rc = 1;

cleanup:

    return rc;
}

static int
read_record(shp_file_t *fh, shp_record_t **precord, size_t *size)
{
    int rc = -1;
    char header_buf[8], *buf;
    size_t record_number, content_length;
    size_t record_size, buf_size;
    struct shp_record_t *record;
    size_t nr;

    nr = fread(header_buf, 1, 8, fh->fp);
    fh->num_bytes += nr;
    if (ferror(fh->fp)) {
        shp_set_error(fh, "Cannot read record header");
        goto cleanup;
    }
    if (feof(fh->fp)) {
        /* Reached end of file. */
        rc = 0;
        goto cleanup;
    }
    if (nr != 8) {
        shp_set_error(fh, "Expected record header of %zu bytes, got %zu",
                      (size_t) 8, nr);
        errno = EINVAL;
        goto cleanup;
    }

    record_number = shp_be32_to_uint32(&header_buf[0]);

    content_length = shp_be32_to_uint32(&header_buf[4]);
    if (content_length < 2) {
        shp_set_error(fh, "Content length %zu is invalid in record %zu",
                      content_length, record_number);
        errno = EINVAL;
        goto cleanup;
    }

    record_size = 2 * content_length;

    record = *precord;
    buf_size = sizeof(*record) + record_size;
    if (record == NULL || *size < buf_size) {
        record = (shp_record_t *) realloc(record, buf_size);
        if (record == NULL) {
            shp_set_error(fh, "Cannot allocate %zu bytes for record %zu",
                          buf_size, record_number);
            goto cleanup;
        }
        *precord = record;
        *size = buf_size;
    }

    buf = ((char *) record) + sizeof(*record);

    nr = fread(buf, 1, record_size, fh->fp);
    fh->num_bytes += nr;
    if (ferror(fh->fp)) {
        shp_set_error(fh, "Cannot read record %zu", record_number);
        goto cleanup;
    }
    if (nr != record_size) {
        shp_set_error(fh,
                      "Expected record of %zu bytes, got %zu in record %zu",
                      record_size, nr, record_number);
        errno = EINVAL;
        goto cleanup;
    }

    record->record_number = record_number;
    record->record_size = record_size;
    record->shape_type = (shp_shpt_t) shp_le32_to_int32(&buf[0]);
    switch (record->shape_type) {
    case SHPT_NULL:
        rc = 1;
        break;
    case SHPT_POINT:
        rc = get_point(fh, buf, record);
        break;
    case SHPT_MULTIPOINT:
        rc = get_multipoint(fh, buf, record);
        break;
    case SHPT_MULTIPOINTM:
        rc = get_multipointm(fh, buf, record);
        break;
    case SHPT_POLYLINE:
        rc = get_polyline(fh, buf, record);
        break;
    case SHPT_POLYGON:
        rc = get_polygon(fh, buf, record);
        break;
    case SHPT_POINTM:
        rc = get_pointm(fh, buf, record);
        break;
    default:
        shp_set_error(fh, "Shape type %d is unknown in record %zu",
                      (int) record->shape_type, record_number);
        errno = EINVAL;
        break;
    }

cleanup:

    return rc;
}

int
shp_read_record(shp_file_t *fh, shp_record_t **precord)
{
    int rc;
    shp_record_t *record = NULL;
    size_t buf_size = 0;

    assert(fh != NULL);
    assert(fh->fp != NULL);
    assert(precord != NULL);

    rc = read_record(fh, &record, &buf_size);
    if (rc <= 0) {
        free(record);
        record = NULL;
    }

    *precord = record;

    return rc;
}

int
shp_seek_record(shp_file_t *fh, size_t file_offset, shp_record_t **precord)
{
    int rc = -1;
    shp_record_t *record = NULL;
    size_t buf_size = 0;

    assert(fh != NULL);
    assert(fh->fp != NULL);
    assert(precord != NULL);

    /* The largest possible file offset is 8GB minus 12 bytes for a null
     * shape.  The offset may be further limited by LONG_MAX on 32-bit
     * systems. */
    if (file_offset > LONG_MAX ||
        fseek(fh->fp, (long) file_offset, SEEK_SET) != 0) {
        shp_set_error(fh, "Cannot set file position to %zu\n", file_offset);
        goto cleanup;
    }

    rc = read_record(fh, &record, &buf_size);
    if (rc <= 0) {
        free(record);
        record = NULL;
    }

cleanup:

    *precord = record;

    return rc;
}

int
shp_read(shp_file_t *fh, shp_header_callback_t handle_header,
         shp_record_callback_t handle_record)
{
    int rc = -1, rc2;
    shp_header_t header;
    shp_record_t *record = NULL;
    size_t buf_size;
    size_t file_offset;

    assert(fh != NULL);
    assert(fh->fp != NULL);
    assert(handle_header != NULL);
    assert(handle_record != NULL);

    rc2 = shp_read_header(fh, &header);
    if (rc2 == 0) {
        /* Reached end of file. */
        rc = 0;
    }
    if (rc2 <= 0) {
        goto cleanup;
    }

    rc2 = (*handle_header)(fh, &header);
    if (rc2 == 0) {
        /* Stop processing. */
        rc = 0;
    }
    if (rc2 <= 0) {
        goto cleanup;
    }

    /* Preallocate a big record. */
    buf_size = SHP_MIN_BUF_SIZE;
    record = (shp_record_t *) malloc(buf_size);
    if (record == NULL) {
        shp_set_error(fh, "Cannot allocate %zu bytes", buf_size);
        goto cleanup;
    }

    for (;;) {
        file_offset = fh->num_bytes;

        rc2 = read_record(fh, &record, &buf_size);
        if (rc2 == 0) {
            /* Reached end of file. */
            rc = 0;
        }
        if (rc2 <= 0) {
            goto cleanup;
        }

        rc2 = (*handle_record)(fh, &header, record, file_offset);
        if (rc2 == 0) {
            /* Stop processing. */
            rc = 0;
        }
        if (rc2 <= 0) {
            goto cleanup;
        }
    }

cleanup:

    free(record);

    return rc;
}
