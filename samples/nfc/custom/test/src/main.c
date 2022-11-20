#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

void setUp(void)
{}

void tearDown(void)
{}

void test_mutual0(void)
{
	TEST_ASSERT(false);
}


extern int unity_main(void);

void main(void)
{
	/* use the runner from test_runner_generate() */
	(void)unity_main();
}