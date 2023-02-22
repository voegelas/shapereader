/*
 * Read ESRI shapefiles
 *
 * Copyright (C) 2023 Andreas Vögele
 *
 * This library is free software; you can redistribute it and/or modify it
 * under either the terms of the ISC License or the same terms as Perl.
 */

/* SPDX-License-Identifier: ISC OR Artistic-1.0-Perl OR GPL-1.0-or-later */

/**
 * @file
 */

#ifndef _SHAPEREADER_SHP_MULTIPOINTM_H
#define _SHAPEREADER_SHP_MULTIPOINTM_H

#include "shp-box.h"
#include "shp-pointm.h"
#include "shp-range.h"
#include <stddef.h>

/**
 * MultiPointM
 *
 * A MultiPointM is a set of points with one measure per point.
 *
 * A measure is some value, for example a temperature, that is associated
 * with a point.
 */
typedef struct shp_multipointm_t {
    shp_box_t box;             /**< Bounding box */
    size_t num_points;         /**< Number of points */
    const char *_points;       /* Points */
    shp_range_t measure_range; /**< Bounding measure range */
    const char *_measures;     /* Measures */
} shp_multipointm_t;

/**
 * Get a PointM
 *
 * Gets a point and a measure from a set of points.
 *
 * @b Example
 *
 * @code{.c}
 * // Iterate over all points
 * size_t i;
 * shp_pointm_t pointm;
 *
 * for (i = 0; i < multipointm->num_points; ++i) {
 *   shp_multipointm_pointm(multipoint, i, &pointm);
 * }
 * @endcode
 *
 * @memberof shp_multipointm_t
 * @param multipointm a shp_multipointm_t structure.
 * @param point_num a zero-based point number.
 * @param[out] pointm a shp_pointm_t structure.
 */
extern void shp_multipointm_pointm(const shp_multipointm_t *multipointm,
                                   size_t point_num, shp_pointm_t *pointm);

#endif
