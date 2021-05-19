//
//  main.cpp
//  tests
//
//  Created by Louis D. Langholtz on 5/19/21.
//

#include "gtest/gtest.h"

#include <fstream>

#include "../library/byte_swap.hpp"
#include "../library/stiffer.hpp"

TEST(byte_swap, are_swapped)
{
    EXPECT_NE(stiffer::byte_swap(5), 5);
}

TEST(get_file_context, throws_if_seek_fails)
{
    std::fstream fstream("nonesuch");
    EXPECT_THROW(stiffer::get_file_context(fstream), std::runtime_error);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
