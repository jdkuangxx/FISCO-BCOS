#include <string_view>
#include <vector>
#include <string>
#include <iostream>
#include <string_view>
#include<gtest/gtest.h>
#include "libtool/Buffer.h"

using namespace std; 
using namespace alpha;

TEST(Buffer, TestWriteBuffer)
{
    ReverseBuffer buffer;
    std::string testData = "Hello world, jdd 666";
    buffer.set_min_block_size(5);

    buffer << (int)5;
    buffer << (int)6;
    buffer << testData;

    ASSERT_EQ(buffer.size(), sizeof(int) * 2 + testData.size());

    ReverseBufferWapper buffer_(buffer, 0);

    std::string str;
    int ret1 = 0;
    int ret2 = 0;
    
    buffer_.read_string(str, testData.size()).read_int(ret1).read_int(ret2);

    std::cout << str << std::endl;
    ASSERT_TRUE(str == testData);

    ASSERT_TRUE(ret1 == 6);
    std::cout << ret1 << std::endl;
    
    ASSERT_TRUE(ret2 == 5);
    std::cout << ret2 << std::endl;
}

TEST(Buffer, TestReadBuffer)
{    
    OrdinalBuffer buffer;
    std::string testData = "Hello world, jdd 666";
    buffer.set_min_block_size(5);
    
    buffer << (int)5;
    buffer << (int)6;
    buffer << testData;

    ASSERT_EQ(buffer.size(), sizeof(int) * 2 + testData.size());

    std::string str;
    int ret1 = 0;
    int ret2 = 0;

    OrdinalBufferWapper buffer_(buffer, 0);
    buffer_.read_int(ret1).read_int(ret2).read_string(str, testData.size());
    
    ASSERT_TRUE(ret1 == 5);
    std::cout << ret1 << std::endl;

    ASSERT_TRUE(ret2 == 6);
    std::cout << ret2 << std::endl;

    ASSERT_TRUE(str == testData);
    std::cout << str << std::endl;
}

TEST(Buffer, TestWrite2ReadBuffer)
{    
    ReverseBuffer wBuffer;
    OrdinalBuffer rBuffer;
    std::string testData = "Hello world, jdd 666";
    wBuffer.set_min_block_size(5);
    rBuffer.set_min_block_size(5);

    wBuffer << (int)5;
    wBuffer << (int)6;
    wBuffer << testData;


    rBuffer.write_buffer(wBuffer);
    ASSERT_EQ(wBuffer.size(), sizeof(int) * 2 + testData.size());
    ASSERT_EQ(rBuffer.size(), sizeof(int) * 2 + testData.size());

    ssize_t offset = 0;
    std::string str(testData.size(), 0);
    ASSERT_TRUE(rBuffer.read_string(str, offset) == testData.size() && str == testData);
    std::cout << str << std::endl;

    offset += testData.size();
    int ret = 0;
    ASSERT_TRUE(rBuffer.read_int(ret, offset) && ret == 6);
    std::cout << ret << std::endl;
    
    offset += sizeof(int);
    ASSERT_TRUE(rBuffer.read_int(ret, offset) && ret == 5);
    std::cout << ret << std::endl;
}

TEST(Buffer, TestWrite2WriteBuffer) {

    ReverseBuffer wBuffer1;
    ReverseBuffer wBuffer2;
    std::string testData = "Hello world, jdd 666";
    wBuffer1.set_min_block_size(5);
    wBuffer2.set_min_block_size(5);

    wBuffer1 << (int)5;
    wBuffer1 << (int)6;
    wBuffer1 << testData;


    wBuffer2.write_buffer(wBuffer1);
    ASSERT_EQ(wBuffer1.size(), sizeof(int) * 2 + testData.size());
    ASSERT_EQ(wBuffer2.size(), sizeof(int) * 2 + testData.size());


    ssize_t offset = 0;
    std::string str(testData.size(), 0);
    ASSERT_TRUE(wBuffer2.read_string(str, offset) == testData.size() && str == testData);
    std::cout << str << std::endl;

    offset += testData.size();
    int ret = 0;
    ASSERT_TRUE(wBuffer2.read_int(ret, offset) && ret == 6);
    std::cout << ret << std::endl;
    
    offset += sizeof(int);
    ASSERT_TRUE(wBuffer2.read_int(ret, offset) && ret == 5);
    std::cout << ret << std::endl;
}

TEST(Buffer, TestRead2WriteBuffer) {
    OrdinalBuffer rBuffer;
    ReverseBuffer wBuffer;
    std::string testData = "Hello world, jdd 666";
    rBuffer.set_min_block_size(5);
    wBuffer.set_min_block_size(5);

    rBuffer << (int)5;
    rBuffer << (int)6;
    rBuffer << testData;
    
    wBuffer.write_buffer(rBuffer);
    ASSERT_EQ(rBuffer.size(), sizeof(int) * 2 + testData.size());
    ASSERT_EQ(wBuffer.size(), sizeof(int) * 2 + testData.size());

    int ret = 0;
    ssize_t offset = 0;
    ASSERT_TRUE(wBuffer.read_int(ret, offset) && ret == 5);
    std::cout << ret << std::endl;

    offset += sizeof(int);
    ASSERT_TRUE(wBuffer.read_int(ret, offset) && ret == 6);
    std::cout << ret << std::endl;

    offset += sizeof(int);
    std::string str(testData.size(), 0);
    ASSERT_TRUE(wBuffer.read_string(str, offset) == testData.size() && str == testData);
    std::cout << str << std::endl;
}

TEST(Buffer, TestRead2ReadBuffer) {
    OrdinalBuffer rBuffer1;
    OrdinalBuffer rBuffer2;
    std::string testData = "Hello world, jdd 666";
    rBuffer1.set_min_block_size(5);
    rBuffer2.set_min_block_size(5);

    rBuffer1 << (int)5;
    rBuffer1 << (int)6;
    rBuffer1 << testData;
    
    rBuffer2.write_buffer(rBuffer1);
    ASSERT_EQ(rBuffer1.size(), sizeof(int) * 2 + testData.size());
    ASSERT_EQ(rBuffer2.size(), sizeof(int) * 2 + testData.size());

    int ret = 0;
    ssize_t offset = 0;
    ASSERT_TRUE(rBuffer2.read_int(ret, offset) && ret == 5);
    std::cout << ret << std::endl;

    offset += sizeof(int);
    ASSERT_TRUE(rBuffer2.read_int(ret, offset) && ret == 6);
    std::cout << ret << std::endl;

    offset += sizeof(int);
    std::string str(testData.size(), 0);
    ASSERT_TRUE(rBuffer2.read_string(str, offset) == testData.size() && str == testData);
    std::cout << str << std::endl;
}