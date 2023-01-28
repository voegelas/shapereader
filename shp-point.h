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

#ifndef _SHAPEREADER_SHP_POINT_H
#define _SHAPEREADER_SHP_POINT_H

/**
 * Point
 *
 * A location in a two-dimensional coordinate plane.
 */
typedef struct shp_point_t {
    double x; /**< The horizontal position */
    double y; /**< The vertical position */
} shp_point_t;

/**
 * Check whether three points lie on a straight line
 *
 * Determines whether three points lie on a straight line.
 *
 * @memberof shp_point_t
 * @param point the first point.
 * @param a the second point.
 * @param b the third point.
 * @param epsilon an error factor, see @cite machine_epsilon for more
 *                information.
 * @retval 1 if the points lie on a straight line.
 * @retval 0 if the points do not lie on a straight line.
 */
extern int shp_point_is_collinear(const shp_point_t *point,
                                  const shp_point_t *a, const shp_point_t *b,
                                  double epsilon);

/**
 * Check whether a point lies between two points on a straight line
 *
 * Determines whether a point lies between two points on a straight line.
 *
 * @memberof shp_point_t
 * @param point a point.
 * @param a the start point.
 * @param b the end point.
 * @param epsilon an error factor, see @cite machine_epsilon for more
 *                information.
 * @retval 1 if the point lies between @c a and @c b (inclusive).
 * @retval 0 if the point does not lie between @c a and @c b.
 */
extern int shp_point_is_between(const shp_point_t *point,
                                const shp_point_t *a, const shp_point_t *b,
                                double epsilon);

#endif
