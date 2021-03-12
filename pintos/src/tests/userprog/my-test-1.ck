# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
my-test-1: exit(12345)
EOF
pass;
