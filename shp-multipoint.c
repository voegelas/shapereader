/*
 * Read ESRI shapefiles
 *
 * Copyright (C) 2023 Andreas Vögele
 *
 * This library is free software; you can redistribute it and/or modify it
 * under either the terms of the ISC License or the same terms as Perl.
 */

/* SPDX-License-Identifier: ISC OR Artistic-1.0-Perl OR GPL-1.0-or-later */

#include "shp-multipoint.h"
#include "convert.h"
#include <assert.h>
#include <stddef.h>

void
shp_multipoint_point(const shp_multipoint_t *multipoint, int32_t point_num,
                     shp_point_t *point)
{
    const char *buf;

    assert(multipoint != NULL);
    assert(point_num >= 0);
    assert(point_num < multipoint->num_points);
    assert(point != NULL);

    buf = multipoint->_points + 16 * point_num;
    point->x = shp_le64_to_double(&buf[0]);
    point->y = shp_le64_to_double(&buf[8]);
}
