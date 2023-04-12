#include "../shapereader.h"
#include "tap.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

int tests_planned = 0;
int tests_run = 0;
int tests_failed = 0;

const shp_header_t *header;
const shp_record_t *record;
const shp_polygon_t *polygon;

size_t record_number;

size_t num_bytes;
size_t file_size;

/*
 * Header tests
 */

static int
test_file_code(void)
{
    return header->file_code == 9994;
}

static int
test_file_length(void)
{
    return header->file_size == file_size;
}

static int
test_entire_file_read(void)
{
    return num_bytes == file_size;
}

static int
test_version(void)
{
    return header->version == 1000;
}

static int
test_shape_type(void)
{
    return header->shape_type == SHP_TYPE_POLYGON;
}

static int
test_x_min(void)
{
    return header->x_min == -180.0;
}

static int
test_y_min(void)
{
    return header->y_min == -90.0;
}

static int
test_x_max(void)
{
    return header->x_max == 180.0;
}

static int
test_y_max(void)
{
    return header->y_max == 90.0;
}

/*
 * Rectangle tests
 */

static int
test_is_polygon(void)
{
    return record->shape_type == SHP_TYPE_POLYGON;
}

static int
test_is_inside(void)
{
    shp_point_t point = {0.5, 0.5};
    return shp_point_in_polygon(&point, polygon) == 1;
}

static int
test_is_outside(void)
{
    shp_point_t point = {0.1, 0.5};
    return shp_point_in_polygon(&point, polygon) == 0;
}

static int
test_is_on_top_edge(void)
{
    shp_point_t point = {0.5, 0.8};
    return shp_point_in_polygon(&point, polygon) == -1;
}

static int
test_is_on_bottom_edge(void)
{
    shp_point_t point = {0.5, 0.2};
    return shp_point_in_polygon(&point, polygon) == -1;
}

static int
test_is_on_left_edge(void)
{
    shp_point_t point = {0.2, 0.5};
    return shp_point_in_polygon(&point, polygon) == -1;
}

static int
test_is_on_right_edge(void)
{
    shp_point_t point = {0.8, 0.5};
    return shp_point_in_polygon(&point, polygon) == -1;
}

static int
test_is_outside_box(void)
{
    shp_point_t point = {1.1, 0.5};
    return shp_point_in_polygon(&point, polygon) == 0;
}

/*
 * Triangle tests
 */

static int
test_has_two_parts(void)
{
    return polygon->num_parts == 2;
}

static int
test_has_eight_points(void)
{
    return polygon->num_points == 8;
}

static int
test_is_inside_with_hole(void)
{
    shp_point_t point = {0.3, 0.3};
    return shp_point_in_polygon(&point, polygon) == 1;
}

static int
test_is_outside_with_hole(void)
{
    shp_point_t point = {0.3, 0.7};
    return shp_point_in_polygon(&point, polygon) == 0;
}

static int
test_is_in_the_hole(void)
{
    shp_point_t point = {0.5, 0.5};
    return shp_point_in_polygon(&point, polygon) == 0;
}

static int
test_is_on_inside_edge(void)
{
    shp_point_t point = {0.45, 0.4};
    return shp_point_in_polygon(&point, polygon) == -1;
}

static int
test_is_on_outside_egde(void)
{
    shp_point_t point = {0.65, 0.2};
    return shp_point_in_polygon(&point, polygon) == -1;
}

/*
 * Location tests
 */

static int
test_is_los_angeles(void)
{
    shp_point_t point = {-122.35007, 47.650499};
    return shp_point_in_polygon(&point, polygon) == 1;
}

static int
test_is_africa_juba(void)
{
    shp_point_t point = {28.0, 9.5}; /* Disputed area */
    return shp_point_in_polygon(&point, polygon) == 1;
}

static int
test_is_africa_khartoum(void)
{
    shp_point_t point = {28.0, 9.5}; /* Disputed area */
    return shp_point_in_polygon(&point, polygon) == 1;
}

static int
test_is_oslo(void)
{
    shp_point_t point = {10.757933, 59.911491};
    return shp_point_in_polygon(&point, polygon) == 1;
}

static int
handle_shp_header(shp_file_t *fh, const shp_header_t *h)
{
    header = h;
    record_number = 0;
    ok(test_file_code, "file code is 9994");
    ok(test_file_length, "file length matches file size");
    ok(test_version, "version is 1000");
    ok(test_shape_type, "shape type is polygon");
    ok(test_x_min, "x_min is set");
    ok(test_y_min, "y_min is set");
    ok(test_x_max, "x_max is set");
    ok(test_y_max, "y_max is set");
    return 1;
}

static int
handle_shp_record(shp_file_t *fh, const shp_header_t *h,
                  const shp_record_t *r, size_t file_offset)
{
    header = h;
    record = r;
    polygon = &r->shape.polygon;
    switch (record_number) {
    case 0:
        ok(test_is_polygon, "shape is polygon");
        ok(test_is_inside, "point is inside");
        ok(test_is_outside, "point is outside");
        ok(test_is_on_top_edge, "point is on top edge");
        ok(test_is_on_bottom_edge, "point is on bottom edge");
        ok(test_is_on_left_edge, "point is on left edge");
        ok(test_is_on_right_edge, "point is on right edge");
        ok(test_is_outside_box, "point is outside bounding box");
        break;
    case 1:
        ok(test_has_two_parts, "polygon has two parts");
        ok(test_has_eight_points, "polygon has eight points");
        ok(test_is_inside_with_hole, "point is inside polygon with hole");
        ok(test_is_outside_with_hole, "point is outside polygon with hole");
        ok(test_is_in_the_hole, "point is in the hole");
        ok(test_is_on_inside_edge, "point is on inside edge");
        ok(test_is_on_outside_egde, "point is on outside edge");
        break;
    case 2:
        ok(test_is_los_angeles, "location is in America/Los_Angeles");
        break;
    case 3:
        ok(test_is_africa_juba, "location is in Africa/Juba");
        break;
    case 4:
        ok(test_is_africa_khartoum, "location is in Africa/Khartoum");
        break;
    case 5:
        ok(test_is_oslo, "location is in Europe/Oslo");
        break;
    }
    ++record_number;
    return 1;
}

int
main(int argc, char *argv[])
{
    const char *filename = "polygon.shp";
    FILE *fp;
    shp_file_t fh;

    plan(28);

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "# Cannot open file \"%s\": %s\n", filename,
                strerror(errno));
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    shp_init_file(&fh, fp, NULL);
    if (shp_read(&fh, handle_shp_header, handle_shp_record) == -1) {
        fprintf(stderr, "# Cannot read file \"%s\": %s\n", filename,
                fh.error);
    }

    num_bytes = fh.num_bytes;
    ok(test_entire_file_read, "entire file has been read");

    fclose(fp);

    done_testing();
}
