/*
 * Read ESRI shapefiles
 *
 * Copyright (C) 2023 Andreas Vögele
 *
 * This library is free software; you can redistribute it and/or modify it
 * under either the terms of the ISC License or the same terms as Perl.
 */

/* SPDX-License-Identifier: ISC OR Artistic-1.0-Perl OR GPL-1.0-or-later */

#include "shp-point.h"
#include <assert.h>
#include <stddef.h>

#define ABS(d) ((d) < 0.0 ? (-d) : (d))

static int
are_collinear(double x1, double y1, double x2, double y2, double x3,
              double y3, double epsilon)
{
    double p = (y1 - y2) * (x1 - x3) - (y1 - y3) * (x1 - x2);
    return ABS(p) <= epsilon;
}

int
shp_point_is_collinear(const shp_point_t *point, const shp_point_t *a,
                       const shp_point_t *b, double epsilon)
{
    assert(point != NULL);
    assert(a != NULL);
    assert(b != NULL);

    return are_collinear(point->x, point->y, a->x, a->y, b->x, b->y, epsilon);
}

int
shp_point_is_between(const shp_point_t *point, const shp_point_t *a,
                     const shp_point_t *b, double epsilon)
{
    double x1, y1, x2, y2, x3, y3;

    assert(point != NULL);
    assert(a != NULL);
    assert(b != NULL);

    x1 = a->x;
    y1 = a->y;
    x2 = point->x;
    y2 = point->y;
    x3 = b->x;
    y3 = b->y;

    if (!((x1 <= x2 && x2 <= x3) || (x3 <= x2 && x2 <= x1))) {
        return 0;
    }

    if (!((y1 <= y2 && y2 <= y3) || (y3 <= y2 && y2 <= y1))) {
        return 0;
    }

    return are_collinear(x1, y1, x2, y2, x3, y3, epsilon);
}
