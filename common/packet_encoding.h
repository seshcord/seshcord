#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "packet_schema.h"
#include "malloc_group.h"

int encode_from_schema( void *, enum packet_items *, int, char *, int, int );
int decode_from_schema( void *, enum packet_items *, int, char *, int, int, malloc_group * );
