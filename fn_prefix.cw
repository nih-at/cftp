@ bindable functions for prefix argument.

@(fn_prefix.fn@)
; fn_prefix
@<functions@>

@u
#include "directory.h"
#include "functions.h"


@ functions for entering digits

@d<functions@>
  { fn_prefix_0, "prefix-0", FN_PRE, "prefix digit 0" }
  { fn_prefix_1, "prefix-1", FN_PRE, "prefix digit 1" }
  { fn_prefix_2, "prefix-2", FN_PRE, "prefix digit 2" }
  { fn_prefix_3, "prefix-3", FN_PRE, "prefix digit 3" }
  { fn_prefix_4, "prefix-4", FN_PRE, "prefix digit 4" }
  { fn_prefix_5, "prefix-5", FN_PRE, "prefix digit 5" }
  { fn_prefix_6, "prefix-6", FN_PRE, "prefix digit 6" }
  { fn_prefix_7, "prefix-7", FN_PRE, "prefix digit 7" }
  { fn_prefix_8, "prefix-8", FN_PRE, "prefix digit 8" }
  { fn_prefix_9, "prefix-9", FN_PRE, "prefix digit 9" }

@u
int
fn_prefix_0()
{
	add_prefix(0);
}

int
fn_prefix_1()
{
	add_prefix(1);
}

int
fn_prefix_2()
{
	add_prefix(2);
}

int
fn_prefix_3()
{
	add_prefix(3);
}

int
fn_prefix_4()
{
	add_prefix(4);
}

int
fn_prefix_5()
{
	add_prefix(5);
}

int
fn_prefix_6()
{
	add_prefix(6);
}

int
fn_prefix_7()
{
	add_prefix(7);
}

int
fn_prefix_8()
{
	add_prefix(8);
}

int
fn_prefix_9()
{
	add_prefix(9);
}


