#include "../shapereader.h"
#include "tap.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define CITY 0
#define LATITUDE 1
#define LONGITUDE 2
#define IS_CAPITAL 3

int tests_planned = 0;
int tests_run = 0;
int tests_failed = 0;

const dbf_header_t *header;
const dbf_record_t *record;

size_t record_number;

/*
 * Header tests
 */

static int
test_database_version(void)
{
    return header->version == DBF_VERSION_DBASE2;
}

static int
test_num_records(void)
{
    return header->num_records == 3;
}

static int
test_header_size(void)
{
    return header->header_size == 521;
}

static int
test_record_size(void)
{
    return header->record_size == 24;
}

static int
test_num_fields(void)
{
    return header->num_fields == 4;
}

static int
test_field_city(void)
{
    dbf_field_t *field = &header->fields[CITY];
    return (strcmp(field->name, "CITY") == 0) &&
           (field->type == DBF_TYPE_CHARACTER);
}

static int
test_field_latitude(void)
{
    dbf_field_t *field = &header->fields[LATITUDE];
    return (strcmp(field->name, "LATITUDE") == 0) &&
           (field->type == DBF_TYPE_NUMBER);
}

static int
test_field_longitude(void)
{
    dbf_field_t *field = &header->fields[LONGITUDE];
    return (strcmp(field->name, "LONGITUDE") == 0) &&
           (field->type == DBF_TYPE_NUMBER);
}

static int
test_field_is_capital(void)
{
    dbf_field_t *field = &header->fields[IS_CAPITAL];
    return (strcmp(field->name, "IS_CAPITAL") == 0) &&
           (field->type == DBF_TYPE_LOGICAL);
}

static int
handle_dbf_header(dbf_file_t *fh, const dbf_header_t *h)
{
    header = h;
    record_number = 0;
    ok(test_database_version, "database version matches");
    ok(test_num_records, "number of records is greater than zero");
    ok(test_header_size, "header_size matches");
    ok(test_record_size, "record size is greater than zero");
    ok(test_num_fields, "number of fields matches");
    ok(test_field_city, "city is string");
    ok(test_field_latitude, "latitude is number");
    ok(test_field_longitude, "longitude is number");
    ok(test_field_is_capital, "is_capital is logical");
    header = NULL;
    return 1;
}

/*
 * Record tests
 */

static int
compare_city(const char *city, double lat, double lon, int is_capital)
{
    int ok = 0;
    dbf_field_t *fields = header->fields;
    char *s;
    double x, y;
    int l;

    dbf_record_strtod(record, &fields[LONGITUDE], &x);
    dbf_record_strtod(record, &fields[LATITUDE], &y);
    l = dbf_record_logical(record, &fields[IS_CAPITAL]);
    s = dbf_record_strdup(record, &fields[CITY]);
    if (s != NULL) {
        ok = (strcmp(s, city) == 0) && (x == lon) && (y == lat) &&
             (l == is_capital);
        free(s);
    }
    return ok;
}

static int
test_milan(void)
{
    return compare_city("Milan", 45.4625, 9.1863, 'F');
}

static int
test_naples(void)
{
    return compare_city("Naples", 40.8333, 14.25, 'F');
}

static int
test_rome(void)
{
    return compare_city("Rome", 41.8833, 12.4833, 'T');
}

static int
handle_dbf_record(dbf_file_t *fh, const dbf_header_t *h,
                  const dbf_record_t *r, size_t file_offset)
{
    header = h;
    record = r;
    switch (record_number) {
    case 0:
        ok(test_milan, "test Milan");
        break;
    case 1:
        ok(test_naples, "test Naples");
        break;
    case 2:
        ok(test_rome, "test Rome");
        break;
    }
    ++record_number;
    return 1;
}

int
main(int argc, char *argv[])
{
    const char *filename = "dbase2.dbf";
    FILE *stream;
    dbf_file_t fh;

    plan(12);

    stream = fopen(filename, "rb");
    if (stream == NULL) {
        fprintf(stderr, "# Cannot open file \"%s\": %s\n", filename,
                strerror(errno));
        return 1;
    }

    dbf_init_file(&fh, stream, NULL);

    dbf_set_error(&fh, "%s", "");

    if (dbf_read(&fh, handle_dbf_header, handle_dbf_record) == -1) {
        fprintf(stderr, "# Cannot read file \"%s\": %s\n", filename,
                fh.error);
    }

    fclose(stream);

    done_testing();
}