/*
 * Read ESRI shapefiles
 *
 * Copyright (C) 2023 Andreas Vögele
 *
 * This library is free software; you can redistribute it and/or modify it
 * under either the terms of the ISC License or the same terms as Perl.
 */

/* SPDX-License-Identifier: ISC OR Artistic-1.0-Perl OR GPL-1.0-or-later */

#include "shp-polyline.h"
#include "convert.h"
#include <assert.h>

size_t
shp_polyline_points(const shp_polyline_t *polyline, size_t part_num,
                    size_t *start, size_t *end)
{
    size_t num_points, i, j, m;
    const char *buf;

    assert(polyline != NULL);
    assert(part_num < polyline->num_parts);
    assert(start != NULL);
    assert(end != NULL);

    m = polyline->num_points;

    buf = polyline->_parts + 4 * part_num;
    i = shp_le32_to_int32(&buf[0]);
    if (part_num + 1 < polyline->num_parts) {
        j = shp_le32_to_int32(&buf[4]);
    }
    else {
        j = m;
    }

    *start = i;
    *end = j;

    /* Is the range valid? */
    num_points = 0;
    if (i >= 0 && i < m && j >= 0 && j <= m && i < j) {
        num_points = j - i;
    }

    return num_points;
}

void
shp_polyline_point(const shp_polyline_t *polyline, size_t point_num,
                   shp_point_t *point)
{
    const char *buf;

    assert(polyline != NULL);
    assert(point_num < polyline->num_points);
    assert(point != NULL);

    buf = polyline->_points + 16 * point_num;
    point->x = shp_le64_to_double(&buf[0]);
    point->y = shp_le64_to_double(&buf[8]);
}

int
shp_polyline_point_on_polyline(const shp_polyline_t *polyline,
                               const shp_point_t *point, double epsilon)
{
    size_t parts_count, part_num, i, n;
    shp_point_t a, b;

    assert(polyline != NULL);
    assert(point != NULL);

    if (shp_box_point_in_box(&polyline->box, point) == 0) {
        return 0;
    }

    parts_count = polyline->num_parts;
    for (part_num = 0; part_num < parts_count; ++part_num) {
        if (shp_polyline_points(polyline, part_num, &i, &n) >= 2) {
            shp_polyline_point(polyline, i, &a);
            while (++i < n) {
                shp_polyline_point(polyline, i, &b);
                if (shp_point_is_between(point, &a, &b, epsilon) != 0) {
                    return 1;
                }
                a = b;
            }
        }
    }

    return 0;
}
