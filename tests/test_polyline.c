#include "../shapereader.h"
#include "tap.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

int tests_planned = 0;
int tests_run = 0;
int tests_failed = 0;

shp_header_t shp_header;
shp_record_t *shp_record;
const shp_polyline_t *polyline;

size_t record_number;

/*
 * Main file tests
 */

static int
test_header_shape_type(void)
{
    return shp_header.shape_type == SHPT_POLYLINE;
}

static int
test_record_shape_type(void)
{
    return shp_record->shape_type == SHPT_POLYLINE;
}

static int
test_bbox(void)
{
    const shp_box_t *b = &polyline->box;
    return b->x_min == 1 && b->y_min == 1 && b->x_max == 3 && b->y_max == 3;
}

static int
test_has_two_parts(void)
{
    return polyline->num_parts == 2;
}

static int
test_has_four_points(void)
{
    return polyline->num_points == 4;
}

static int
test_has_six_points(void)
{
    return polyline->num_points == 6;
}

static int
test_points_on_diagonal_cross(void)
{
    int i;
    shp_point_t p[5] = {{1, 1}, {1, 3}, {2, 2}, {3, 1}, {3, 3}};

    for (i = 0; i < 5; ++i) {
        if (shp_polyline_point_on_polyline(polyline, &p[i], 0) == 0) {
            return 0;
        }
    }
    return 1;
}

static int
test_points_on_greek_cross(void)
{
    int i;
    shp_point_t p[5] = {{1, 2}, {2, 1}, {2, 2}, {2, 3}, {3, 2}};

    for (i = 0; i < 5; ++i) {
        if (shp_polyline_point_on_polyline(polyline, &p[i], 0) == 0) {
            return 0;
        }
    }
    return 1;
}

static int
test_point_on_line(void)
{
    shp_point_t p = {1.3, 1.3};
    return shp_polyline_point_on_polyline(polyline, &p, 0) != 0;
}

static int
test_point_not_on_line(void)
{
    shp_point_t p = {1.2999, 1.3};
    return shp_polyline_point_on_polyline(polyline, &p, 0) == 0;
}

static int
test_point_on_line_with_epsilon(void)
{
    shp_point_t p = {1.2999, 1.3};
    return shp_polyline_point_on_polyline(polyline, &p, 9.77e-04) != 0;
}

static int
test_point_left_of_polyline(void)
{
    shp_point_t p = {0.9999, 2};
    return shp_polyline_point_on_polyline(polyline, &p, 0) == 0;
}

static int
test_point_right_of_polyline(void)
{
    shp_point_t p = {3.0001, 2};
    return shp_polyline_point_on_polyline(polyline, &p, 0) == 0;
}

static int
test_point_below_polyline(void)
{
    shp_point_t p = {2, 0.9999};
    return shp_polyline_point_on_polyline(polyline, &p, 0) == 0;
}

static int
test_point_above_polyline(void)
{
    shp_point_t p = {2, 3.0001};
    return shp_polyline_point_on_polyline(polyline, &p, 0) == 0;
}

static void
test_shp(void)
{
    ok(test_record_shape_type, "record shape type is polyline");
    polyline = &shp_record->shape.polyline;
    switch (record_number) {
    case 0:
        ok(test_bbox, "bounding box matches");
        ok(test_has_two_parts, "diagonal cross has two parts");
        ok(test_has_four_points, "diagonal cross has four points");
        ok(test_points_on_diagonal_cross, "points on diagonal cross");
        ok(test_point_on_line, "point on line");
        ok(test_point_not_on_line, "point not on line");
        ok(test_point_on_line_with_epsilon, "point on line with epsilon");
        break;
    case 1:
        ok(test_has_two_parts, "greek cross has two parts");
        ok(test_has_six_points, "greek cross has six points");
        ok(test_points_on_greek_cross, "points on greek cross");
        ok(test_point_left_of_polyline, "point left of polyline");
        ok(test_point_right_of_polyline, "point right of polyline");
        ok(test_point_below_polyline, "point below polyline");
        ok(test_point_above_polyline, "point above polyline");
        break;
    }
}

int
main(int argc, char *argv[])
{
    const char *filename = "polyline.shp";
    FILE *fp;
    shp_file_t fh;

    plan(17);

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "# Cannot open file \"%s\": %s\n", filename,
                strerror(errno));
        return 1;
    }

    shp_init_file(&fh, fp, NULL);

    if (shp_read_header(&fh, &shp_header) > 0) {
        ok(test_header_shape_type, "header shape type is polyline");
        record_number = 0;
        while (shp_read_record(&fh, &shp_record) > 0) {
            test_shp();
            free(shp_record);
            ++record_number;
        }
        fprintf(stderr, "# %s\n", fh.error);
    }

    fclose(fp);

    done_testing();
}
