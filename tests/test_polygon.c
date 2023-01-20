#include "../shapereader.h"
#include "tap.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int tests_planned = 0;
int tests_run = 0;
int tests_failed = 0;

struct shape_index {
    size_t file_offset;
    size_t record_size;
};

struct shape_index *shape_index;

shx_file_t index_fh;

const shp_header_t *header;
const shp_record_t *record;
const shp_polygon_t *polygon;
shp_file_t fh;

const shx_header_t *index_header;
size_t index_file_size;
size_t main_file_size;
size_t record_file_offset;
int32_t record_number;

static size_t
get_file_size(const char *filename)
{
    struct stat statbuf;
    if (stat(filename, &statbuf) == 0) {
        return (size_t) statbuf.st_size;
    }
    return 0;
}

/*
 * Index file tests
 */

static int
test_index_file_size(void)
{
    return index_header->file_size == index_file_size;
}

static int
test_index_file_read(void)
{
    return index_fh.num_bytes == index_file_size;
}

static int
handle_shx_header(shx_file_t *fh, const shx_header_t *h)
{
    record_number = 0;
    index_header = h;
    ok(test_index_file_size, "index file sizes match");
    return 1;
}

static int
handle_shx_record(shx_file_t *fh, const shx_header_t *h,
                  const shx_record_t *r)
{
    shape_index[record_number].file_offset = r->file_offset;
    shape_index[record_number].record_size = r->record_size;
    ++record_number;
    return 1;
}

static int
read_index_file(const char *filename)
{
    FILE *fp;

    index_file_size = get_file_size(filename);
    if (index_file_size < 108) {
        fprintf(stderr, "# File \"%s\" is too small: %zu\n", filename,
                index_file_size);
        return 0;
    }

    shape_index = (struct shape_index *) calloc((index_file_size - 100) / 8,
                                                sizeof(struct shape_index));
    if (shape_index == NULL) {
        fprintf(stderr, "# Cannot allocate index: %s\n", strerror(errno));
        return 0;
    }

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "# Cannot open file \"%s\": %s\n", filename,
                strerror(errno));
        return 0;
    }

    shx_file(&index_fh, fp, NULL);
    if (shx_read(&index_fh, handle_shx_header, handle_shx_record) == -1) {
        fprintf(stderr, "# Cannot read file \"%s\": %s\n", filename,
                index_fh.error);
    }

    ok(test_index_file_read, "entire index file has been read");

    return 1;
}

/*
 * Main file tests
 */

/*
 * Header tests
 */

static int
test_file_code(void)
{
    return header->file_code == 9994;
}

static int
test_main_file_size(void)
{
    return header->file_size == main_file_size;
}

static int
test_main_file_read(void)
{
    return fh.num_bytes == main_file_size;
}

static int
test_version(void)
{
    return header->version == 1000;
}

static int
test_shape_type(void)
{
    return header->shape_type == SHPT_POLYGON;
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
    return record->shape_type == SHPT_POLYGON;
}

static int
test_is_inside(void)
{
    shp_point_t point = {0.5, 0.5};
    return shp_polygon_point_in_polygon(polygon, &point) == 1;
}

static int
test_is_outside(void)
{
    shp_point_t point = {0.1, 0.5};
    return shp_polygon_point_in_polygon(polygon, &point) == 0;
}

static int
test_is_on_top_edge(void)
{
    shp_point_t point = {0.5, 0.8};
    return shp_polygon_point_in_polygon(polygon, &point) == -1;
}

static int
test_is_on_bottom_edge(void)
{
    shp_point_t point = {0.5, 0.2};
    return shp_polygon_point_in_polygon(polygon, &point) == -1;
}

static int
test_is_on_left_edge(void)
{
    shp_point_t point = {0.2, 0.5};
    return shp_polygon_point_in_polygon(polygon, &point) == -1;
}

static int
test_is_on_right_edge(void)
{
    shp_point_t point = {0.8, 0.5};
    return shp_polygon_point_in_polygon(polygon, &point) == -1;
}

static int
test_is_outside_box(void)
{
    shp_point_t point = {1.1, 0.5};
    return shp_polygon_point_in_polygon(polygon, &point) == 0;
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
    return shp_polygon_point_in_polygon(polygon, &point) == 1;
}

static int
test_is_outside_with_hole(void)
{
    shp_point_t point = {0.3, 0.7};
    return shp_polygon_point_in_polygon(polygon, &point) == 0;
}

static int
test_is_in_the_hole(void)
{
    shp_point_t point = {0.5, 0.5};
    return shp_polygon_point_in_polygon(polygon, &point) == 0;
}

static int
test_is_on_inside_edge(void)
{
    shp_point_t point = {0.45, 0.4};
    return shp_polygon_point_in_polygon(polygon, &point) == -1;
}

static int
test_is_on_outside_egde(void)
{
    shp_point_t point = {0.65, 0.2};
    return shp_polygon_point_in_polygon(polygon, &point) == -1;
}

/*
 * Location tests
 */

static int
test_is_los_angeles(void)
{
    shp_point_t point = {-122.35007, 47.650499};
    return shp_polygon_point_in_polygon(polygon, &point) == 1;
}

static int
test_is_africa_juba(void)
{
    shp_point_t point = {28.0, 9.5}; /* Disputed area */
    return shp_polygon_point_in_polygon(polygon, &point) == 1;
}

static int
test_is_africa_khartoum(void)
{
    shp_point_t point = {28.0, 9.5}; /* Disputed area */
    return shp_polygon_point_in_polygon(polygon, &point) == 1;
}

static int
test_is_oslo(void)
{
    shp_point_t point = {10.757933, 59.911491};
    return shp_polygon_point_in_polygon(polygon, &point) == 1;
}

static int
test_record_numbers_match(void)
{
    return record->record_number == record_number;
}

static int
test_file_offsets_match(void)
{
    return record_file_offset == shape_index[record_number].file_offset;
}

static int
test_record_sizes_match(void)
{
    return record->record_size == shape_index[record_number].record_size;
}

static int
handle_shp_header(shp_file_t *fh, const shp_header_t *h)
{
    record_number = 0;
    header = h;
    ok(test_file_code, "file code is 9994");
    ok(test_main_file_size, "main file sizes match");
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
    record_file_offset = file_offset;
    switch (record_number) {
    case 0:
        ok(test_record_numbers_match, "1st record number matches");
        ok(test_file_offsets_match, "1st file offset matches");
        ok(test_record_sizes_match, "1st content length matches");
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
        ok(test_record_numbers_match, "2nd record number matches");
        ok(test_file_offsets_match, "2nd file offset matches");
        ok(test_record_sizes_match, "2nd content length matches");
        ok(test_has_two_parts, "polygon has two parts");
        ok(test_has_eight_points, "polygon has eight points");
        ok(test_is_inside_with_hole, "point is inside polygon with hole");
        ok(test_is_outside_with_hole, "point is outside polygon with hole");
        ok(test_is_in_the_hole, "point is in the hole");
        ok(test_is_on_inside_edge, "point is on inside edge");
        ok(test_is_on_outside_egde, "point is on outside edge");
        break;
    case 2:
        ok(test_record_numbers_match, "3rd record number matches");
        ok(test_file_offsets_match, "3rd file offset matches");
        ok(test_record_sizes_match, "3rd content length matches");
        ok(test_is_los_angeles, "location is in America/Los_Angeles");
        break;
    case 3:
        ok(test_record_numbers_match, "4th record number matches");
        ok(test_file_offsets_match, "4th file offset matches");
        ok(test_record_sizes_match, "4th content length matches");
        ok(test_is_africa_juba, "location is in Africa/Juba");
        break;
    case 4:
        ok(test_record_numbers_match, "5th record number matches");
        ok(test_file_offsets_match, "5th file offset matches");
        ok(test_record_sizes_match, "5th content length matches");
        ok(test_is_africa_khartoum, "location is in Africa/Khartoum");
        break;
    case 5:
        ok(test_record_numbers_match, "6th record number matches");
        ok(test_file_offsets_match, "6th file offset matches");
        ok(test_record_sizes_match, "6th content length matches");
        ok(test_is_oslo, "location is in Europe/Oslo");
        break;
    }
    ++record_number;
    return 1;
}

static int
read_main_file(const char *filename)
{
    FILE *fp;

    main_file_size = get_file_size(filename);
    if (main_file_size < 108) {
        fprintf(stderr, "# File \"%s\" is too small: %zu\n", filename,
                main_file_size);
        return 0;
    }

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "# Cannot open file \"%s\": %s\n", filename,
                strerror(errno));
        return 0;
    }

    shp_file(&fh, fp, NULL);
    if (shp_read(&fh, handle_shp_header, handle_shp_record) == -1) {
        fprintf(stderr, "# Cannot read file \"%s\": %s\n", filename,
                fh.error);
    }

    ok(test_main_file_read, "entire main file has been read");

    return 1;
}

void
seek_test(void)
{
    shx_record_t ir;
    shp_record_t *r = NULL;

    record_number = 5;
    if (shx_seek_record(&index_fh, record_number, &ir) > 0) {
        if (shp_seek_record(&fh, ir.file_offset, &r) > 0) {
            record = r;
            polygon = &r->shape.polygon;
            ok(test_is_oslo, "location is in Europe/Oslo");
            free(r);
        }
        else {
            fprintf(stderr, "# Cannot set file position in main file: %s\n",
                    index_fh.error);
        }
    }
    else {
        fprintf(stderr, "# Cannot set file position in index file: %s\n",
                index_fh.error);
    }
}

int
main(int argc, char *argv[])
{
    const char *datadir = getenv("datadir");

    plan(49);

    if (datadir == NULL) {
        fprintf(stderr,
                "# The environment variable \"datadir\" is not set\n");
        return 1;
    }

    if (chdir(datadir) == -1) {
        fprintf(stderr, "# Cannot change directory to \"%s\": %s\n", datadir,
                strerror(errno));
        return 1;
    }

    if (!read_index_file("polygon.shx")) {
        free(shape_index);
        return 1;
    }

    if (!read_main_file("polygon.shp")) {
        free(shape_index);
        return 1;
    }

    seek_test();

    fclose(fh.fp);
    fclose(index_fh.fp);

    free(shape_index);

    done_testing();
}
