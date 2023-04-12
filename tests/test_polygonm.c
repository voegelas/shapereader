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
const shp_polygonm_t *polygonm;

size_t record_number;

/*
 * Main file tests
 */

static int
test_header_shape_type(void)
{
    return shp_header.shape_type == SHP_TYPE_POLYGONM;
}

static int
test_record_shape_type(void)
{
    return shp_record->shape_type == SHP_TYPE_POLYGONM;
}

static int
test_bbox(void)
{
    return polygonm->x_min == 1 && polygonm->y_min == 1 &&
           polygonm->x_max == 2 && polygonm->y_max == 2;
}

static int
test_has_one_part(void)
{
    return polygonm->num_parts == 1;
}

static int
test_has_six_points(void)
{
    return polygonm->num_points == 5;
}

static int
test_points_match(void)
{
    size_t i, j, n;
    shp_pointm_t p;
    const shp_pointm_t points[5] = {
        {1, 1, 1}, {1, 2, 2}, {2, 2, 3}, {2, 1, 4}, {1, 1, 5}};

    j = 0;
    shp_polygonm_points(polygonm, 0, &i, &n);
    while (i < n) {
        shp_polygonm_pointm(polygonm, i, &p);
        if (p.x != points[j].x || p.y != points[j].y || p.m != points[j].m) {
            return 0;
        }
        ++j;
        ++i;
    }
    return 1;
}

static void
test_shp(void)
{
    ok(test_record_shape_type, "record shape type is polygonm");
    polygonm = &shp_record->shape.polygonm;
    switch (record_number) {
    case 0:
        ok(test_bbox, "bounding box matches");
        ok(test_has_one_part, "polygon has one part");
        ok(test_has_six_points, "polygon has five points");
        ok(test_points_match, "points match");
        break;
    }
}

int
main(int argc, char *argv[])
{
    const char *filename = "polygonm.shp";
    FILE *fp;
    shp_file_t fh;

    plan(6);

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "# Cannot open file \"%s\": %s\n", filename,
                strerror(errno));
        return 1;
    }

    shp_init_file(&fh, fp, NULL);

    if (shp_read_header(&fh, &shp_header) > 0) {
        ok(test_header_shape_type, "header shape type is polygonm");
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
