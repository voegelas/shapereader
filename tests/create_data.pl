#!/usr/bin/perl

use strict;
use warnings;
use utf8;

use Encode                qw(encode);
use File::Spec::Functions qw(catfile);
use List::Util 1.54       qw(any reduce reductions);
use Time::Piece;

my $SHPT_NULL        = 0;
my $SHPT_POINT       = 1;
my $SHPT_POLYLINE    = 3;
my $SHPT_POLYGON     = 5;
my $SHPT_MULTIPOINT  = 8;
my $SHPT_POINTZ      = 11;
my $SHPT_POLYLINEZ   = 13;
my $SHPT_POLYGONZ    = 15;
my $SHPT_MULTIPOINTZ = 18;
my $SHPT_POINTM      = 21;
my $SHPT_POLYLINEM   = 23;
my $SHPT_POLYGONM    = 25;
my $SHPT_MULTIPOINTM = 28;
my $SHPT_MULTIPATCH  = 31;

my %ENCODING_FOR = (
    0x00 => 'UTF-8',
    0x13 => 'cp932',
    0x4d => 'cp936',
    0x4e => 'cp949',
    0x4f => 'cp950',
    0x57 => 'cp1252',
    0x58 => 'cp1252',
);

sub pack_dbf_field {
    my %h = (
        name           => q{},
        type           => 'C',
        length         => 0,
        decimal_places => 0,
        @_
    );

    my $bytes = pack 'a11aC20', $h{name}, $h{type}, (0) x 4, $h{length},
        $h{decimal_places}, (0) x 14;
    return $bytes;
}

sub get_field_size {
    my $field = shift;

    my $size = $field->{length};
    if ($field->{type} eq 'C') {
        $size |= ($field->{decimal_places} // 0) << 8;
    }
    return $size;
}

sub pack_dbf_header {
    my (%h) = @_;

    my @reserved = (0) x 20;
    $reserved[17] = $h{ldid};

    my @packed_fields = map { pack_dbf_field(%{$_}) } @{$h{fields}};

    $h{header_size} = reduce { $a + length $b } 33, @packed_fields;
    $h{record_size} = reduce { $a + get_field_size($b) } 1, @{$h{fields}};

    my $bytes = pack 'C4LS2C20', $h{version}, $h{year} - 1900, $h{month},
        $h{day}, $h{num_records}, $h{header_size}, $h{record_size}, @reserved;
    for my $field (@packed_fields) {
        $bytes .= $field;
    }
    $bytes .= "\r";

    return $bytes;
}

sub get_datetime {
    my $tp = shift;

    my $jd   = $tp->julian_day + 0.5;
    my $day  = int $jd;
    my $usec = 86400000 * ($jd - $day);

    return ($day, $usec);
}

sub write_cpg {
    my (%args) = @_;

    my $file     = $args{file};
    my $encoding = $args{encoding};

    open my $fh, '>:raw', $file or die "Can't open $file: $!";
    print {$fh} $encoding;
    close $fh;
    return;
}

sub write_dbf {
    my (%args) = @_;

    my $file   = $args{file};
    my %header = (
        version     => 0x03,
        year        => 2022,
        month       => 11,
        day         => 6,
        num_records => 0,
        header_size => 0,
        record_size => 0,
        ldid        => 0x57,
        fields      => [],
        %{$args{header}}
    );
    my @records = @{$args{records}};

    $header{num_records} = scalar @records;

    my $is_foxpro = any { $header{version} == $_ } (0x30, 0x31, 0x32);

    my $encoding = $ENCODING_FOR{$header{ldid}};
    my @fields   = @{$header{fields}};

    open my $fh, '>:raw', $file or die "Can't open $file: $!";
    print {$fh} pack_dbf_header(%header);
    for my $record (@records) {
        my @values  = @{$record};
        my $deleted = shift @values;
        print {$fh} pack 'A', $deleted;
        my $i = 0;
        for my $value (@values) {
            my $field  = $fields[$i];
            my $name   = $field->{name};
            my $type   = $field->{type};
            my $places = $field->{decimal_places} // 0;
            my $size   = get_field_size($field);
            my $bytes;
            if ($type eq 'C') {
                $bytes = pack "A$size",
                    defined $value ? encode($encoding, $value) : q{};
            }
            elsif ($type eq 'D') {
                $bytes
                    = pack "A$size", defined $value
                    ? sprintf "%0*s", $size, $value->ymd(q{})
                    : q{};
            }
            elsif ($type eq 'F' || $type eq 'N') {
                $bytes
                    = pack "A$size",
                    defined $value
                    ? sprintf "%*.*f", $size, $places, $value
                    : q{};
            }
            elsif ($type eq 'I' || $type eq '+') {
                $bytes = pack "l<", defined $value ? $value : 0;
            }
            elsif ($type eq 'L') {
                $bytes = pack "A", defined $value ? $value : q{ };
            }
            elsif ($type eq 'O' || ($type eq 'B' && $is_foxpro)) {
                $bytes = pack "d<", defined $value ? $value : 0.0;
            }
            elsif ($type eq 'T' || $type eq '@') {
                $bytes = pack "l<l<",
                    defined $value ? get_datetime($value) : 0 x 2;
            }
            elsif ($type eq 'Y') {
                $bytes = pack "q<", defined $value ? $value * 10**$places : 0;
            }
            elsif ($type eq '0' && $name eq '_NullFlags') {
                $bytes = pack "C$size", defined $value ? $value : 0 x $size;
            }
            else {
                die "Data type '$type' not supported";
            }
            print {$fh} $bytes;
            ++$i;
        }
    }
    print {$fh} "\x1a";
    close $fh;
    return;
}

sub pack_shp_header {
    my (%h) = @_;

    my $bytes = pack 'N7L2d<8', $h{file_code}, (0) x 5, $h{file_length},
        $h{version}, $h{shape_type}, $h{x_min}, $h{y_min}, $h{x_max},
        $h{y_max},   $h{z_min},      $h{z_max}, $h{m_min}, $h{m_max};

    return $bytes;
}

sub pack_null {
    my %h = (
        record_number => 0,
        shape_type    => $SHPT_NULL,
        @_
    );

    my $bytes = pack 'N2L', $h{record_number}, 2, $h{shape_type};

    return $bytes;
}

sub pack_point {
    my %h = (
        record_number => 0,
        shape_type    => $SHPT_POINT,
        point         => [0.0, 0.0],
        @_
    );

    my $bytes = pack 'N2Ld<2', $h{record_number}, 10, $h{shape_type},
        @{$h{point}}[0 .. 1];

    return $bytes;
}

sub pack_pointm {
    my %h = (
        record_number => 0,
        shape_type    => $SHPT_POINTM,
        point         => [0.0, 0.0, 0.0],
        @_
    );

    my $bytes = pack 'N2Ld<3', $h{record_number}, 14, $h{shape_type},
        @{$h{point}}[0 .. 2];

    return $bytes;
}

sub pack_pointz {
    my %h = (
        record_number => 0,
        shape_type    => $SHPT_POINTZ,
        point         => [0.0, 0.0, 0.0, 0.0],
        @_
    );

    my $bytes = pack 'N2Ld<4', $h{record_number}, 18, $h{shape_type},
        @{$h{point}}[0 .. 3];

    return $bytes;
}

sub pack_multipoint {
    my %h = (
        record_number => 0,
        shape_type    => $SHPT_MULTIPOINT,
        box           => [0.0, 0.0, 0.0, 0.0],
        points        => [],
        @_
    );

    my @points = @{$h{points}};
    my @xy     = map { $_->[0], $_->[1] } @points;

    my $points_count = scalar @points;
    my $xy_count     = scalar @xy;

    my $content_length = (40 + 16 * $points_count) / 2;

    my $bytes = pack "N2Ld<4L" . "d<${xy_count}",
        $h{record_number}, $content_length, $h{shape_type},
        @{$h{box}}[0 .. 3], $points_count, @xy;

    return $bytes;
}

sub pack_multipointm {
    my %h = (
        record_number => 0,
        shape_type    => $SHPT_MULTIPOINTM,
        box           => [0.0, 0.0, 0.0, 0.0],
        m_range       => [0.0, 0.0],
        points        => [],
        @_
    );

    my @points = @{$h{points}};
    my @xy     = map { $_->[0], $_->[1] } @points;
    my @m      = map { $_->[2] } @points;

    my $points_count = scalar @points;
    my $xy_count     = scalar @xy;

    my $content_length = (56 + 24 * $points_count) / 2;

    my $bytes
        = pack "N2Ld<4L" . "d<${xy_count}" . "d<2d<${points_count}",
        $h{record_number}, $content_length, $h{shape_type},
        @{$h{box}}[0 .. 3], $points_count, @xy, @{$h{m_range}}[0 .. 1], @m;

    return $bytes;
}

sub pack_multipointz {
    my %h = (
        record_number => 0,
        shape_type    => $SHPT_MULTIPOINTZ,
        box           => [0.0, 0.0, 0.0, 0.0],
        z_range       => [0.0, 0.0],
        m_range       => [0.0, 0.0],
        points        => [],
        @_
    );

    my @points = @{$h{points}};
    my @xy     = map { $_->[0], $_->[1] } @points;
    my @z      = map { $_->[2] } @points;
    my @m      = map { $_->[3] } @points;

    my $points_count = scalar @points;
    my $xy_count     = scalar @xy;

    my $content_length = (72 + 32 * $points_count) / 2;

    my $bytes
        = pack "N2Ld<4L"
        . "d<${xy_count}"
        . "d<2d<${points_count}"
        . "d<2d<${points_count}",
        $h{record_number}, $content_length, $h{shape_type},
        @{$h{box}}[0 .. 3], $points_count, @xy,
        @{$h{z_range}}[0 .. 1], @z, @{$h{m_range}}[0 .. 1], @m;

    return $bytes;
}

sub pack_polyline {
    my %h = (
        record_number => 0,
        shape_type    => $SHPT_POLYLINE,
        box           => [0.0, 0.0, 0.0, 0.0],
        parts         => [],
        @_
    );

    my @parts = @{$h{parts}};

    my @parts_index = reductions { $a + scalar @{$b} } 0, @parts;
    pop @parts_index;

    my @points = map { @{$_} } @parts;
    my @xy     = map { $_->[0], $_->[1] } @points;

    my $parts_count  = scalar @parts;
    my $points_count = scalar @points;
    my $xy_count     = scalar @xy;

    my $content_length = (44 + 4 * $parts_count + 16 * $points_count) / 2;

    my $bytes
        = pack "N2Ld<4L2L${parts_count}" . "d<${xy_count}",
        $h{record_number}, $content_length, $h{shape_type},
        @{$h{box}}[0 .. 3], $parts_count, $points_count, @parts_index,
        @xy;

    return $bytes;
}

sub pack_polylinem {
    my %h = (
        record_number => 0,
        shape_type    => $SHPT_POLYLINEM,
        box           => [0.0, 0.0, 0.0, 0.0],
        m_range       => [0.0, 0.0],
        parts         => [],
        @_
    );

    my @parts       = @{$h{parts}};
    my $parts_count = scalar @parts;

    my @parts_index = reductions { $a + scalar @{$b} } 0, @parts;
    pop @parts_index;

    my @points = map { @{$_} } @parts;
    my @xy     = map { $_->[0], $_->[1] } @points;
    my @m      = map { $_->[2] } @points;

    my $points_count = scalar @points;
    my $xy_count     = scalar @xy;

    my $content_length = (60 + 4 * $parts_count + 24 * $points_count) / 2;

    my $bytes
        = pack "N2Ld<4L2L${parts_count}"
        . "d<${xy_count}"
        . "d<2d<${points_count}",
        $h{record_number}, $content_length, $h{shape_type},
        @{$h{box}}[0 .. 3], $parts_count, $points_count, @parts_index,
        @xy, @{$h{m_range}}[0 .. 1], @m;

    return $bytes;
}

sub pack_polylinez {
    my %h = (
        record_number => 0,
        shape_type    => $SHPT_POLYLINEZ,
        box           => [0.0, 0.0, 0.0, 0.0],
        z_range       => [0.0, 0.0],
        m_range       => [0.0, 0.0],
        parts         => [],
        @_
    );

    my @parts       = @{$h{parts}};
    my $parts_count = scalar @parts;

    my @parts_index = reductions { $a + scalar @{$b} } 0, @parts;
    pop @parts_index;

    my @points = map { @{$_} } @parts;
    my @xy     = map { $_->[0], $_->[1] } @points;
    my @z      = map { $_->[2] } @points;
    my @m      = map { $_->[3] } @points;

    my $points_count = scalar @points;
    my $xy_count     = scalar @xy;

    my $content_length = (76 + 4 * $parts_count + 32 * $points_count) / 2;

    my $bytes
        = pack "N2Ld<4L2L${parts_count}"
        . "d<${xy_count}"
        . "d<2d<${points_count}"
        . "d<2d<${points_count}",
        $h{record_number}, $content_length, $h{shape_type},
        @{$h{box}}[0 .. 3], $parts_count, $points_count, @parts_index,
        @xy, @{$h{z_range}}[0 .. 1], @z, @{$h{m_range}}[0 .. 1], @m;

    return $bytes;
}

sub pack_polygon {
    my %h = (
        record_number => 0,
        shape_type    => $SHPT_POLYGON,
        box           => [0.0, 0.0, 0.0, 0.0],
        parts         => [],
        @_
    );

    return pack_polyline(%h);
}

sub pack_polygonm {
    my %h = (
        record_number => 0,
        shape_type    => $SHPT_POLYGONM,
        box           => [0.0, 0.0, 0.0, 0.0],
        m_range       => [0.0, 0.0],
        parts         => [],
        @_
    );

    return pack_polylinem(%h);
}

sub pack_polygonz {
    my %h = (
        record_number => 0,
        shape_type    => $SHPT_POLYGONZ,
        box           => [0.0, 0.0, 0.0, 0.0],
        z_range       => [0.0, 0.0],
        m_range       => [0.0, 0.0],
        parts         => [],
        @_
    );

    return pack_polylinez(%h);
}

my %pack_shp_record = (
    $SHPT_NULL        => \&pack_null,
    $SHPT_POINT       => \&pack_point,
    $SHPT_POLYLINE    => \&pack_polyline,
    $SHPT_POLYGON     => \&pack_polygon,
    $SHPT_MULTIPOINT  => \&pack_multipoint,
    $SHPT_POINTZ      => \&pack_pointz,
    $SHPT_POLYLINEZ   => \&pack_polylinez,
    $SHPT_POLYGONZ    => \&pack_polygonz,
    $SHPT_MULTIPOINTZ => \&pack_multipointz,
    $SHPT_POINTM      => \&pack_pointm,
    $SHPT_POLYLINEM   => \&pack_polylinem,
    $SHPT_POLYGONM    => \&pack_polygonm,
    $SHPT_MULTIPOINTM => \&pack_multipointm,
    $SHPT_MULTIPATCH  => undef,
);

sub pack_shp_record {
    my (%shape) = @_;

    return $pack_shp_record{$shape{shape_type}}->(%shape);
}

sub write_shp_and_shx {
    my (%args) = @_;

    my $shp_file = $args{shp_file};
    my $shx_file = $args{shx_file};
    my %header   = (
        file_code   => 9994,
        file_length => 0,
        version     => 1000,
        shape_type  => 0,
        x_min       => 0.0,
        y_min       => 0.0,
        x_max       => 0.0,
        y_max       => 0.0,
        z_min       => 0.0,
        z_max       => 0.0,
        m_min       => 0.0,
        m_max       => 0.0,
        %{$args{header}}
    );
    my @shapes = @{$args{shapes}};

    my $record_number = 0;
    for my $shape (@shapes) {
        $shape->{record_number} = $record_number;
        ++$record_number;
    }

    my @packed_shapes = map { pack_shp_record(%{$_}) } @shapes;

    my $size = reduce { $a + length $b } 0, @packed_shapes;
    $header{file_length} = (100 + $size) / 2;

    open my $shp_fh, '>:raw', $shp_file or die "Can't open $shp_file: $!";
    open my $shx_fh, '>:raw', $shx_file or die "Can't open $shx_file: $!";
    print {$shp_fh} pack_shp_header(%header);
    $header{file_length} = (100 + 8 * @shapes) / 2;
    print {$shx_fh} pack_shp_header(%header);
    for my $record (@packed_shapes) {
        my $file_offset    = tell($shp_fh) / 2;
        my $content_length = substr $record, 4, 4;
        my $index_record   = pack('N', $file_offset) . $content_length;
        print {$shp_fh} $record;
        print {$shx_fh} $index_record;
    }
    close $shp_fh;
    close $shx_fh;
    return;
}

sub strptime {
    my $time = shift;

    return Time::Piece->strptime($time, "%Y-%m-%d %H:%M:%S");
}

#
# types.dbf
#

write_dbf(
    file   => catfile(qw(data types.dbf)),
    header => {
        version => 0x30,
        fields  => [
            {   name   => 'FESTIVAL',
                type   => 'C',
                length => 32,
            },
            {   name   => 'FROM',
                type   => 'D',
                length => 8,
            },
            {   name   => 'TO',
                type   => 'T',
                length => 8,
            },
            {   name   => 'LOCATION',
                type   => 'C',
                length => 254,
            },
            {   name           => 'LATITUDE',
                type           => 'F',
                length         => 10,
                decimal_places => 4,
            },
            {   name   => 'LONGITUDE',
                type   => 'B',
                length => 8,
            },
            {   name   => 'BANDS',
                type   => 'I',
                length => 4,
            },
            {   name           => 'ADMISSION',
                type           => 'N',
                length         => 4,
                decimal_places => 0,
            },
            {   name           => 'BEER_PRICE',
                type           => 'Y',
                length         => 8,
                decimal_places => 4,
            },
            {   name           => 'FOOD_PRICE',
                type           => 'N',
                length         => 8,
                decimal_places => 4,
            },
            {   name   => 'SOLD_OUT',
                type   => 'L',
                length => 1,
            },
        ],
    },
    records => [
        [   q{ },
            'Graspop Metal Meeting',
            strptime('2022-06-16 00:00:00'),
            strptime('2022-06-19 23:59:59'),
            'Dessel',
            51.2395,
            5.1132,
            129,
            249,
            5.50,
            8.25,
            'T',
        ],
        [   q{*},   undef, undef, undef, q{*} x 254, undef, -179.999999,
            -2**31, undef, -2**31 / 10_000,
            -1.23,  undef,
        ],
    ]
);

#
# point.shp
#

write_dbf(
    file   => catfile(qw(data point.dbf)),
    header => {
        fields => [
            {   name   => 'name',
                type   => 'C',
                length => 12,
            },
            {   name           => 'geoname_id',
                type           => 'N',
                length         => 12,
                decimal_places => 0,
            },
        ],
    },
    records => [
        [q{ }, 'Freiburg',  2925177],
        [q{ }, 'Karlsruhe', 2892794],
        [q{ }, 'Mannheim',  2873891],
        [q{ }, 'Stuttgart', 2825297],
    ]
);

write_shp_and_shx(
    shp_file => catfile(qw(data point.shp)),
    shx_file => catfile(qw(data point.shx)),
    header   => {
        shape_type => $SHPT_POINT,
        x_min      => 7.8522,
        y_min      => 47.9959,
        x_max      => 9.1770,
        y_max      => 49.4891,
    },
    shapes => [
        {   shape_type => $SHPT_POINT,
            point      => [7.8522, 47.9959],
        },
        {   shape_type => $SHPT_POINT,
            point      => [8.4044, 49.0094],
        },
        {   shape_type => $SHPT_POINT,
            point      => [8.4669, 49.4891],
        },
        {   shape_type => $SHPT_POINT,
            point      => [9.1770, 48.7823],
        },
    ]
);

#
# pointm.shp
#

write_dbf(
    file   => catfile(qw(data pointm.dbf)),
    header => {
        fields => [
            {   name           => 'id',
                type           => 'N',
                length         => 10,
                decimal_places => 0,
            },
            {   name   => 'name',
                type   => 'C',
                length => 12,
            },
            {   name   => 'date',
                type   => 'D',
                length => 8,
            },
        ],
    },
    records => [
        [q{ }, 1, 'Buenos Aires', strptime('2020-02-29 12:00:00')],
        [q{ }, 2, 'Los Angeles',  strptime('2013-07-18 12:00:00')],
        [q{ }, 3, 'Oslo',         strptime('2010-12-31 12:00:00')],
        [q{ }, 4, 'Sidney',       strptime('2016-06-03 12:00:00')],
    ]
);

write_shp_and_shx(
    shp_file => catfile(qw(data pointm.shp)),
    shx_file => catfile(qw(data pointm.shx)),
    header   => {
        shape_type => $SHPT_POINTM,
        x_min      => -118.2437,
        y_min      => -34.6132,
        x_max      => 151.2073,
        y_max      => 59.9127,
    },
    shapes => [
        {   shape_type => $SHPT_POINTM,
            point      => [-58.3772, -34.6132, 29],
        },
        {   shape_type => $SHPT_POINTM,
            point      => [-118.2437, 34.0522, 26],
        },
        {   shape_type => $SHPT_POINTM,
            point      => [10.7461, 59.9127, -13],
        },
        {   shape_type => $SHPT_POINTM,
            point      => [151.2073, -33.8679, 17],
        },
    ]
);

#
# pointz.shp
#

write_cpg(
    file     => catfile(qw(data pointz.cpg)),
    encoding => 'UTF-8',
);

write_dbf(
    file   => catfile(qw(data pointz.dbf)),
    header => {
        ldid   => 0x00,
        fields => [
            {   name           => 'id',
                type           => 'N',
                length         => 10,
                decimal_places => 0,
            },
            {   name   => 'name',
                type   => 'C',
                length => 16,
            },
        ],
    },
    records => [
        [q{ }, 1, 'Großglockner'],
        [q{ }, 2, 'Mont Blanc'],
        [q{ }, 3, 'Zugspitze'],
    ]
);

write_shp_and_shx(
    shp_file => catfile(qw(data pointz.shp)),
    shx_file => catfile(qw(data pointz.shx)),
    header   => {
        shape_type => $SHPT_POINTZ,
        x_min      => 6.864325,
        y_min      => 45.832544,
        x_max      => 12.6939,
        y_max      => 47.42122,
        z_min      => 2962.06,
        z_max      => 4807.81,
        m_min      => 25.8,
        m_max      => 2812,
    },
    shapes => [
        {   shape_type => $SHPT_POINTZ,
            point      => [12.6939, 47.074531, 3798, 175],
        },
        {   shape_type => $SHPT_POINTZ,
            point      => [6.864325, 45.832544, 4807.81, 2812],
        },
        {   shape_type => $SHPT_POINTZ,
            point      => [10.9863, 47.42122, 2962.06, 25.8],
        },
    ]
);

#
# multipoint.shp
#

write_dbf(
    file   => catfile(qw(data multipoint.dbf)),
    header => {
        ldid   => 0x57,
        fields => [{
            name   => 'AREA',
            type   => 'C',
            length => 16,
        }],
    },
    records => [[q{ }, 'Bärensee'], [q{ }, 'Schönbuch']]
);

write_shp_and_shx(
    shp_file => catfile(qw(data multipoint.shp)),
    shx_file => catfile(qw(data multipoint.shx)),
    header   => {
        shape_type => $SHPT_MULTIPOINT,
        x_min      => 8.9973,
        y_min      => 48.5671,
        x_max      => 9.0911,
        y_max      => 48.7719,
    },
    shapes => [
        {   shape_type => $SHPT_MULTIPOINT,
            box        => [9.0909, 48.7642, 9.0911, 48.7719],
            points     => [
                [9.0909, 48.7642],    # Grillplatz am Wapitiweg
                [9.0911, 48.7719],    # Pappelgartengrillhütte
            ]
        },
        {   shape_type => $SHPT_MULTIPOINT,
            box        => [8.9973, 48.5671, 9.0611, 48.6091],
            points     => [
                [8.9973, 48.5851],    # Feuerstelle Ziegelweiher
                [9.0611, 48.5763],    # Grillstelle Brühlweiher
                [9.0607, 48.5671],    # Zwergeles Feuerstelle
                [9.0504, 48.6091],    # Feuerstelle Zweites Häusle
            ]
        },
    ]
);

#
# multipointm.shp
#

write_dbf(
    file   => catfile(qw(data multipointm.dbf)),
    header => {
        year   => 2023,
        month  => 2,
        day    => 23,
        fields => [{
            name   => 'AREA',
            type   => 'C',
            length => 16,
        }],
    },
    records => [[q{ }, 'Africa'], [q{ }, 'Europe']]
);

write_shp_and_shx(
    shp_file => catfile(qw(data multipointm.shp)),
    shx_file => catfile(qw(data multipointm.shx)),
    header   => {
        shape_type => $SHPT_MULTIPOINTM,
        x_min      => -21.8277,
        y_min      => -33.9189,
        x_max      => 37.6184,
        y_max      => 64.1283,
        m_min      => -5,
        m_max      => 31,
    },
    shapes => [
        {   shape_type => $SHPT_MULTIPOINTM,
            box        => [-0.2059, -33.9189, 31.2333, 30.0333],
            m_range    => [20, 31],
            points     => [
                [-0.2059, 5.6148,   31],    # Accra
                [31.2333, 30.0333,  20],    # Cairo
                [18.4233, -33.9189, 23],    # Cape Town
            ]
        },
        {   shape_type => $SHPT_MULTIPOINTM,
            box        => [-21.8277, 38.7369, 37.6184, 64.1283],
            m_range    => [-5, 15],
            points     => [
                [-9.1427,  38.7369, 15],    # Lisbon
                [37.6184,  55.7512, -5],    # Moscow
                [-21.8277, 64.1283, 1],     # Reykjavík
            ]
        },
    ]
);

#
# multipointz.shp
#

write_dbf(
    file   => catfile(qw(data multipointz.dbf)),
    header => {
        year   => 2023,
        month  => 4,
        day    => 1,
        fields => [{
            name   => 'AREA',
            type   => 'C',
            length => 16,
        }],
    },
    records => [[q{ }, 'North America'], [q{ }, 'South America']]
);

write_shp_and_shx(
    shp_file => catfile(qw(data multipointz.shp)),
    shx_file => catfile(qw(data multipointz.shx)),
    header   => {
        shape_type => $SHPT_MULTIPOINTZ,
        x_min      => -151.007708,
        y_min      => -32.653333,
        x_max      => -68.54176,
        y_max      => 63.068515,
        z_min      => 4392,
        z_max      => 6961,
        m_min      => 96.67,
        m_max      => 16536,
    },
    shapes => [
        {   shape_type => $SHPT_MULTIPOINTZ,
            box        => [-151.007708, 19.027778, -98.623056, 63.068515],
            z_range    => [4392,        6190],
            m_range    => [142,         7450],
            points     => [
                [-151.007708, 63.068515, 6190, 7450],    # Denali
                [-121.760556, 46.853056, 4392, 1177],    # Mount Rainier
                [-98.623056,  19.027778, 5452, 142],     # Popocatépetl
            ]
        },
        {   shape_type => $SHPT_MULTIPOINTZ,
            box        => [-78.437131, -32.653333, -68.54176, -0.684067],
            z_range    => [5897,       6961],
            m_range    => [96.67,      16536],
            points     => [
                [-70.011667, -32.653333, 6961, 16536],    # Aconcagua
                [-78.437131, -0.684067,  5897, 96.67],    # Cotopaxi
                [-68.54176,  -27.10928,  6893, 630],      # Ojos del Salado
            ]
        },
    ]
);

#
# polyline.shp
#

write_dbf(
    file   => catfile(qw(data polyline.dbf)),
    header => {
        fields => [{
            name   => 'id',
            type   => 'N',
            length => 10,
        }],
    },
    records => [[q{ }, 1], [q{ }, 2]]
);

write_shp_and_shx(
    shp_file => catfile(qw(data polyline.shp)),
    shx_file => catfile(qw(data polyline.shx)),
    header   => {
        shape_type => $SHPT_POLYLINE,
        x_min      => 1,
        y_min      => 1,
        x_max      => 3,
        y_max      => 3,
    },
    shapes => [
        {   shape_type => $SHPT_POLYLINE,
            box        => [1, 1, 3, 3],
            parts      => [[[1, 1], [3, 3]], [[1, 3], [3, 1]]]
        },
        {   shape_type => $SHPT_POLYLINE,
            box        => [1, 1, 3, 3],
            parts      => [[[1, 2], [2, 2], [2, 3]], [[2, 1], [2, 2], [3, 2]]]
        },
    ]
);

#
# polylinem.shp
#

write_dbf(
    file   => catfile(qw(data polylinem.dbf)),
    header => {
        fields => [{
            name   => 'id',
            type   => 'N',
            length => 10,
        }],
    },
    records => [[q{ }, 1]]
);

write_shp_and_shx(
    shp_file => catfile(qw(data polylinem.shp)),
    shx_file => catfile(qw(data polylinem.shx)),
    header   => {
        shape_type => $SHPT_POLYLINEM,
        x_min      => 1,
        y_min      => 1,
        x_max      => 4,
        y_max      => 2,
        m_min      => 1,
        m_max      => 6,
    },
    shapes => [
        {   shape_type => $SHPT_POLYLINEM,
            box        => [1, 1, 4, 2],
            m_range    => [1, 6],
            parts      => [[
                [1, 1, 1], [2, 1, 2], [2, 2, 3], [3, 2, 4],
                [3, 1, 5], [4, 1, 6]
            ]]
        },
    ]
);

#
# polylinez.shp
#

write_dbf(
    file   => catfile(qw(data polylinez.dbf)),
    header => {
        fields => [{
            name   => 'id',
            type   => 'N',
            length => 10,
        }],
    },
    records => [[q{ }, 1]]
);

write_shp_and_shx(
    shp_file => catfile(qw(data polylinez.shp)),
    shx_file => catfile(qw(data polylinez.shx)),
    header   => {
        shape_type => $SHPT_POLYLINEZ,
        x_min      => 8.975675,
        y_min      => 48.746122,
        x_max      => 8.976038,
        y_max      => 48.746420,
        z_min      => 477.5,
        z_max      => 493.3,
        m_min      => 0.0,
        m_max      => 2.02,
    },
    shapes => [
        {   shape_type => $SHPT_POLYLINEZ,
            box        => [8.975675, 48.746122, 8.976038, 48.746420],
            z_range    => [477.5,    493.3],
            m_range    => [0.0,      2.02],
            parts      => [
                [   [8.975817, 48.746274, 493.2, 0.0],
                    [8.975824, 48.746279, 493.3, 0.0],
                    [8.975824, 48.746269, 491.1, 0.15],
                    [8.975806, 48.746263, 488.8, 0.45],
                    [8.975681, 48.746227, 485.6, 2.02],
                    [8.975677, 48.746213, 485.2, 1.53],
                    [8.975675, 48.746135, 482.3, 1.36],
                    [8.975675, 48.746122, 482.1, 1.36],
                ],
                [   [8.975819, 48.746283, 480.3, 0.0],
                    [8.975821, 48.746283, 480.1, 0.26],
                    [8.975826, 48.746284, 479.3, 0.62],
                    [8.975833, 48.746284, 478.1, 0.0],
                    [8.975848, 48.746289, 478.8, 0.6],
                    [8.975943, 48.746341, 478.1, 1.4],
                    [8.975954, 48.746351, 477.5, 1.39],
                    [8.976038, 48.746420, 478.9, 1.43],
                ],
            ]
        },
    ]
);

#
# polygon.shp
#

write_dbf(
    file   => catfile(qw(data polygon.dbf)),
    header => {
        fields => [{
            name   => 'tzid',
            type   => 'C',
            length => 80,
        }],
    },
    records => [
        [q{ }, 'Rectangle'],
        [q{ }, 'Triangle'],
        [q{ }, 'America/Los_Angeles'],
        [q{ }, 'Africa/Juba'],
        [q{ }, 'Africa/Khartoum'],
        [q{ }, 'Europe/Oslo'],
    ]
);

write_shp_and_shx(
    shp_file => catfile(qw(data polygon.shp)),
    shx_file => catfile(qw(data polygon.shx)),
    header   => {
        shape_type => $SHPT_POLYGON,
        x_min      => -180.0,
        y_min      => -90.0,
        x_max      => 180.0,
        y_max      => 90.0,
    },
    shapes => [
        {    # rectangle
            shape_type => $SHPT_POLYGON,
            box        => [0, 0, 1, 1],
            parts      =>
                [[[0.2, 0.2], [0.2, 0.8], [0.8, 0.8], [0.8, 0.2], [0.2, 0.2]]]
        },
        {    # triangle with hole
            shape_type => $SHPT_POLYGON,
            box        => [0, 0, 1, 1],
            parts      => [
                [[0.2, 0.2], [0.5, 0.8], [0.8, 0.2], [0.2, 0.2]],
                [[0.4, 0.4], [0.5, 0.6], [0.6, 0.4], [0.4, 0.4]]
            ]
        },
        {    # America/Los_Angeles
            shape_type => $SHPT_POLYGON,
            box        => [-126, 33, -114, 49],
            parts      =>
                [[[-126, 33], [-126, 49], [-114, 49], [-114, 33], [-126, 33]]]
        },
        {    # Africa/Juba
            shape_type => $SHPT_POLYGON,
            box        => [23.45, 3.49, 35.95, 12.24],
            parts      => [[
                [23.45, 3.49],
                [23.45, 12.24],
                [35.95, 12.24],
                [35.95, 3.49],
                [23.45, 3.49]
            ]]
        },
        {    # Africa/Khartoum
            shape_type => $SHPT_POLYGON,
            box        => [21.81, 8.69, 39.06, 22.22],
            parts      => [[
                [21.81, 8.69],
                [21.81, 22.22],
                [39.06, 22.22],
                [39.06, 8.69],
                [21.81, 8.69]
            ]]
        },
        {    # Europe/Oslo
            shape_type => $SHPT_POLYGON,
            box        => [10, 59, 11, 60],
            parts      => [[[10, 59], [10, 60], [11, 60], [11, 59], [10, 59]]]
        },
    ]
);

#
# polygonm.shp
#

write_dbf(
    file   => catfile(qw(data polygonm.dbf)),
    header => {
        fields => [{
            name   => 'id',
            type   => 'N',
            length => 10,
        }],
    },
    records => [[q{ }, 1]]
);

write_shp_and_shx(
    shp_file => catfile(qw(data polygonm.shp)),
    shx_file => catfile(qw(data polygonm.shx)),
    header   => {
        shape_type => $SHPT_POLYGONM,
        x_min      => 1,
        y_min      => 1,
        x_max      => 2,
        y_max      => 2,
        m_min      => 1,
        m_max      => 5,
    },
    shapes => [
        {   shape_type => $SHPT_POLYGONM,
            box        => [1, 1, 2, 2],
            m_range    => [1, 5],
            parts => [[[1, 1, 1], [1, 2, 2], [2, 2, 3], [2, 1, 4], [1, 1, 5]]]
        },
    ]
);

#
# polygonz.shp
#

write_dbf(
    file   => catfile(qw(data polygonz.dbf)),
    header => {
        fields => [{
            name   => 'id',
            type   => 'N',
            length => 10,
        }],
    },
    records => [[q{ }, 1]]
);

write_shp_and_shx(
    shp_file => catfile(qw(data polygonz.shp)),
    shx_file => catfile(qw(data polygonz.shx)),
    header   => {
        shape_type => $SHPT_POLYGONZ,
        x_min      => 0,
        y_min      => 0,
        x_max      => 1,
        y_max      => 1,
        z_min      => 0,
        z_max      => 1,
        m_min      => 0,
        m_max      => 29,
    },
    shapes => [
        {   shape_type => $SHPT_POLYGONZ,
            box        => [0, 0, 1, 1],
            z_range    => [0, 1],
            m_range    => [0, 29],
            parts      => [
                [   [0, 0, 0, 0],
                    [0, 1, 0, 1],
                    [0, 1, 1, 2],
                    [0, 0, 1, 3],
                    [0, 0, 0, 4]
                ],
                [   [0, 0, 0, 5],
                    [0, 0, 1, 6],
                    [1, 0, 1, 7],
                    [1, 0, 0, 8],
                    [0, 0, 0, 9]
                ],
                [   [0, 0, 1, 20],
                    [0, 1, 1, 21],
                    [1, 1, 1, 22],
                    [1, 0, 1, 23],
                    [0, 0, 1, 24]
                ],
                [   [1, 1, 0, 10],
                    [1, 1, 1, 11],
                    [0, 1, 1, 12],
                    [0, 1, 0, 13],
                    [1, 1, 0, 14]
                ],
                [   [1, 0, 0, 15],
                    [1, 0, 1, 16],
                    [1, 1, 1, 17],
                    [1, 1, 0, 18],
                    [1, 0, 0, 19]
                ],
                [   [0, 0, 0, 25],
                    [0, 1, 0, 26],
                    [1, 1, 0, 27],
                    [1, 0, 0, 28],
                    [0, 0, 0, 29]
                ],
            ]
        },
    ]
);
