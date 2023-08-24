#include <string_view>
#include <vector>
#include <string>
#include <iostream>
#include <string_view>
#include<gtest/gtest.h>
#include "libtool/Buffer.h"

using namespace std; 

TEST(Buffer, TestWriteBuffer)
{
    WriteBuffer buffer;
    std::string testData = "Hello world, jdd 666";
    buffer.set_min_block_size(5);
    buffer.write_int((int)5);
    buffer.write_int((int)6);
    buffer.write_string(testData);

    ASSERT_EQ(buffer.size(), sizeof(int) * 2 + testData.size());

    ssize_t offset = 0;
    std::string str(testData.size(), 0);
    ASSERT_TRUE(buffer.read_string(str, offset) == testData.size() && str == testData);
    std::cout << str << std::endl;

    offset += testData.size();
    int ret = 0;
    ASSERT_TRUE(buffer.read_int(ret, offset) && ret == 6);
    std::cout << ret << std::endl;
    
    offset += sizeof(int);
    ASSERT_TRUE(buffer.read_int(ret, offset) && ret == 5);
    std::cout << ret << std::endl;
}

TEST(Buffer, TestReadBuffer)
{    
    ReadBuffer buffer;
    std::string testData = "Hello world, jdd 666";
    buffer.set_min_block_size(5);
    buffer.write_int((int)5);
    buffer.write_int((int)6);
    buffer.write_string(testData);

    ASSERT_EQ(buffer.size(), sizeof(int) * 2 + testData.size());
    
    int ret = 0;
    ssize_t offset = 0;
    ASSERT_TRUE(buffer.read_int(ret, offset) && ret == 5);
    std::cout << ret << std::endl;

    offset += sizeof(int);
    ASSERT_TRUE(buffer.read_int(ret, offset) && ret == 6);
    std::cout << ret << std::endl;

    offset += sizeof(int);
    std::string str(testData.size(), 0);
    ASSERT_TRUE(buffer.read_string(str, offset) == testData.size() && str == testData);
    std::cout << str << std::endl;
}