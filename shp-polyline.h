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

#ifndef _SHAPEREADER_SHP_POLYLINE_H
#define _SHAPEREADER_SHP_POLYLINE_H

#include "shp-box.h"
#include "shp-point.h"
#include <stddef.h>

/**
 * Polyline
 *
 * A polyline consists of one or more parts.  A part is a connected sequence
 * of of two or more points.  See the "ESRI Shapefile Technical Description"
 * @cite ESRI_shape for more information.
 */
typedef struct shp_polyline_t {
    shp_box_t box;       /**< Bounding box */
    size_t num_parts;    /**< Number of parts */
    size_t num_points;   /**< Total number of points */
    const char *_parts;  /* Index to first point in part */
    const char *_points; /* Points for all parts */
} shp_polyline_t;

/**
 * Get the points that form a part
 *
 * Gets the indices for the points specified by @p part_num.
 *
 * @memberof shp_polyline_t
 * @param polyline a polyline.
 * @param part_num a zero-based part number.
 * @param[out] start the range start.
 * @param[out] end the range end (exclusive).
 * @return the number of points in the part.  At least 2 if the part is valid.
 *
 * @see shp_polyline_point
 */
extern size_t shp_polyline_points(const shp_polyline_t *polyline,
                                  size_t part_num, size_t *start,
                                  size_t *end);

/**
 * Get a point
 *
 * Gets a point that belongs to a polyline.
 *
 * @b Example
 *
 * @code{.c}
 * // Iterate over all parts and points
 * size_t part_num, i, n;
 * shp_point_t point;
 *
 * for (part_num = 0; part_num < polyline->num_parts; ++part_num) {
 *   shp_polyline_points(polyline, part_num, &i, &n);
 *   while (i < n) {
 *     shp_polyline_point(polyline, i, &point);
 *     ++i;
 *   }
 * }
 * @endcode
 *
 * @memberof shp_polyline_t
 * @param polyline a polyline.
 * @param point_num a zero-based point number.
 * @param[out] point a shp_point_t structure.
 *
 * @see shp_polyline_points
 */
extern void shp_polyline_point(const shp_polyline_t *polyline,
                               size_t point_num, shp_point_t *point);

/**
 * Check whether a point is on a polyline
 *
 * Determines whether a point is on a polyline.
 *
 * @memberof shp_polyline_t
 * @param polyline a polyline.
 * @param point a point.
 * @param epsilon an error factor, see @cite machine_epsilon for more
 *                information.
 * @retval 1 if the point is on the polyline.
 * @retval 0 if the point is not on the polyline.
 */
extern int shp_polyline_point_on_polyline(const shp_polyline_t *polyline,
                                          const shp_point_t *point,
                                          double epsilon);

#endif
