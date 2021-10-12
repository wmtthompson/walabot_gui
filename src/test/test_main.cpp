/*
 * test_main.cpp
 *
 *  Created on: Mar 4, 2020
 *      Author: william
 */

#include <gtest/gtest.h>
#include "test_file_writer.h"

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

