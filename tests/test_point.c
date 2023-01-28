#include "../shapereader.h"
#include "tap.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define EPSILON 2.22e-16

int tests_planned = 0;
int tests_run = 0;
int tests_failed = 0;

shp_header_t shp_header;
shp_record_t *shp_record;
const shp_point_t *point;

size_t record_number;

/*
 * Main file tests
 */

static int
test_header_shape_type(void)
{
    return shp_header.shape_type == SHPT_POINT;
}

static int
test_record_shape_type(void)
{
    return shp_record->shape_type == SHPT_POINT;
}

static int
test_freiburg(void)
{
    return point->x == 7.8522 && point->y == 47.9959;
}

static int
test_karlsruhe(void)
{
    return point->x == 8.4044 && point->y == 49.0094;
}

static int
test_mannheim(void)
{
    return point->x == 8.4669 && point->y == 49.4891;
}

static int
test_stuttgart(void)
{
    return point->x == 9.1770 && point->y == 48.7823;
}

static void
test_shp(void)
{
    ok(test_record_shape_type, "record shape type is point");
    point = &shp_record->shape.point;
    switch (record_number) {
    case 0:
        ok(test_freiburg, "location is Freiburg");
        break;
    case 1:
        ok(test_karlsruhe, "location is Karlsruhe");
        break;
    case 2:
        ok(test_mannheim, "location is Mannheim");
        break;
    case 3:
        ok(test_stuttgart, "location is Stuttgart");
        break;
    }
}

static int
test_decimals(void)
{
    shp_point_t p[3] = {{0, 0.3}, {1, 0.6}, {2, 0.9}};
    return shp_point_is_collinear(&p[0], &p[1], &p[2], EPSILON) != 0;
}

static int
test_decimals_no_epsilon(void)
{
    shp_point_t p[3] = {{0, 0.3}, {1, 0.6}, {2, 0.9}};
    return shp_point_is_collinear(&p[0], &p[1], &p[2], 0) == 0;
}

static int
test_rationals(void)
{
    shp_point_t p[3] = {{0, 1.0 / 3.0}, {1, 2.0 / 3.0}, {2, 1}};
    return shp_point_is_collinear(&p[0], &p[1], &p[2], EPSILON) != 0;
}

static int
test_rationals_no_epsilon(void)
{
    shp_point_t p[3] = {{0, 1.0 / 3.0}, {1, 2.0 / 3.0}, {2, 1}};
    return shp_point_is_collinear(&p[0], &p[1], &p[2], 0) == 0;
}

static int
test_integers(void)
{
    shp_point_t p[3] = {{0, 0}, {1, 1}, {2, 2}};
    return shp_point_is_collinear(&p[0], &p[1], &p[2], EPSILON) != 0;
}

static int
test_integers_no_epsilon(void)
{
    shp_point_t p[3] = {{0, 0}, {1, 1}, {2, 2}};
    return shp_point_is_collinear(&p[0], &p[1], &p[2], 0) != 0;
}

static int
test_x_is_x1(void)
{
    shp_point_t p[3] = {{-1, 0}, {-1, 0}, {1, 0}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) != 0;
}

static int
test_x_is_x2(void)
{
    shp_point_t p[3] = {{1, 0}, {-1, 0}, {1, 0}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) != 0;
}

static int
test_x_between_x1_and_x2(void)
{
    shp_point_t p[3] = {{0, 0}, {-1, 0}, {1, 0}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) != 0;
}

static int
test_x_between_x2_and_x1(void)
{
    shp_point_t p[3] = {{0, 0}, {1, 0}, {-1, 0}};
    return shp_point_is_between(&p[0], &p[2], &p[1], 0) != 0;
}

static int
test_x_is_left_of_x1(void)
{
    shp_point_t p[3] = {{-2, 0}, {-1, 0}, {1, 0}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) == 0;
}

static int
test_x_is_right_of_x2(void)
{
    shp_point_t p[3] = {{3, 0}, {-1, 0}, {1, 0}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) == 0;
}

static int
test_x_is_below_x1(void)
{
    shp_point_t p[3] = {{-1, -1}, {-1, 0}, {1, 0}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) == 0;
}

static int
test_x_is_above_x2(void)
{
    shp_point_t p[3] = {{1, 1}, {-1, 0}, {1, 0}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) == 0;
}

static int
test_y_is_y1(void)
{
    shp_point_t p[3] = {{0, -1}, {0, -1}, {0, 1}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) != 0;
}

static int
test_y_is_y2(void)
{
    shp_point_t p[3] = {{0, 1}, {0, -1}, {0, 1}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) != 0;
}

static int
test_y_between_y1_and_y2(void)
{
    shp_point_t p[3] = {{0, 0}, {0, -1}, {0, 1}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) != 0;
}

static int
test_y_between_y2_and_y1(void)
{
    shp_point_t p[3] = {{0, 0}, {0, 1}, {0, -1}};
    return shp_point_is_between(&p[0], &p[2], &p[1], 0) != 0;
}

static int
test_y_is_left_of_y1(void)
{
    shp_point_t p[3] = {{-1, -1}, {0, -1}, {0, 1}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) == 0;
}

static int
test_y_is_right_of_y2(void)
{
    shp_point_t p[3] = {{1, 1}, {0, -1}, {0, 1}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) == 0;
}

static int
test_y_is_below_y1(void)
{
    shp_point_t p[3] = {{0, -2}, {0, -1}, {0, 1}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) == 0;
}

static int
test_y_is_above_y2(void)
{
    shp_point_t p[3] = {{0, 2}, {0, -1}, {0, 1}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) == 0;
}

static int
test_is_between_1(void)
{
    shp_point_t p[3] = {{0, 0}, {-1, -1}, {1, 1}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) != 0;
}

static int
test_is_between_2(void)
{
    shp_point_t p[3] = {{0, 0}, {-1, 1}, {1, -1}};
    return shp_point_is_between(&p[0], &p[1], &p[2], 0) != 0;
}

static int
test_is_not_between(void)
{
    shp_point_t p[3] = {{-1, -1}, {0, 0}, {1, 1}};
    return shp_point_is_collinear(&p[0], &p[1], &p[2], 0) != 0 &&
           shp_point_is_between(&p[0], &p[1], &p[2], 0) == 0;
}

int
main(int argc, char *argv[])
{
    const char *filename = "point.shp";
    FILE *fp;
    shp_file_t fh;

    plan(34);

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "# Cannot open file \"%s\": %s\n", filename,
                strerror(errno));
        return 1;
    }

    shp_init_file(&fh, fp, NULL);

    if (shp_read_header(&fh, &shp_header) > 0) {
        ok(test_header_shape_type, "header shape type is point");
        record_number = 0;
        while (shp_read_record(&fh, &shp_record) > 0) {
            test_shp();
            free(shp_record);
            ++record_number;
        }
        fprintf(stderr, "# %s\n", fh.error);
    }

    fclose(fp);

    ok(test_decimals, "collinear decimals with epsilon");
    ok(test_decimals_no_epsilon, "inexact decimals without epsilon");
    ok(test_rationals, "collinear rationals with epsilon");
    ok(test_rationals_no_epsilon, "inexact rationals without epsilon");
    ok(test_integers, "collinear integers with epsilon");
    ok(test_integers_no_epsilon, "collinear integers without epsilon");

    ok(test_x_is_x1, "x is x1");
    ok(test_x_is_x2, "x is x2");
    ok(test_x_between_x1_and_x2, "x is between x1 and x2");
    ok(test_x_between_x2_and_x1, "x is between x2 and x1");
    ok(test_x_is_left_of_x1, "x is left of x1");
    ok(test_x_is_right_of_x2, "x is right of x2");
    ok(test_x_is_below_x1, "x is below x1");
    ok(test_x_is_above_x2, "x is above x2");

    ok(test_y_is_y1, "y is y1");
    ok(test_y_is_y2, "y is y2");
    ok(test_y_between_y1_and_y2, "y is between y1 and y2");
    ok(test_y_between_y2_and_y1, "y is between y2 and y1");
    ok(test_y_is_left_of_y1, "y is left of y1");
    ok(test_y_is_right_of_y2, "y is right of y2");
    ok(test_y_is_below_y1, "y is below y1");
    ok(test_y_is_above_y2, "y is above y2");

    ok(test_is_between_1, "point is between two points");
    ok(test_is_between_2, "point is between two other points");
    ok(test_is_not_between, "collinear point is not between two points");

    done_testing();
}
