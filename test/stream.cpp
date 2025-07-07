#include "stream.h"
#include <cstdio>
#include <gtest/gtest.h>
#include <fstream>
#include <type_traits>

// Demonstrate some basic assertions.
TEST(StreamTest, BasicTest) {
    FileStream stream ("resource/streamtest.txt");
    EXPECT_EQ(stream.tellg(), 0);

    char c = stream.get();
    EXPECT_EQ(c, 'T');
    EXPECT_EQ(stream.tellg(), 1);
    EXPECT_EQ(stream.getPos(), (FilePos {1, 2, 1}));


    stream.get(c);
    EXPECT_EQ(c, 'h');
    EXPECT_EQ(stream.good(), true);
    EXPECT_EQ(stream.tellg(), 2);
    EXPECT_EQ(stream.eof(), false);

    // check for no side effect
    EXPECT_EQ(stream.peek(), 'i');
    EXPECT_EQ(stream.peek(), 'i');
    EXPECT_EQ(stream.tellg(), 2);
    EXPECT_EQ(stream.getPos(), (FilePos {1, 3, 2}));

    while(!stream.eof()){
        stream.get();
    }
    EXPECT_EQ(stream.eof(), true);
}

TEST(StreamTest, SeekTest){
    FileStream stream ("resource/streamtest.txt");

    stream.seekg(0);
    EXPECT_EQ(stream.getPos(), (FilePos {1, 1, 0}));
    EXPECT_EQ(stream.peek(), 'T');
    EXPECT_EQ(stream.get(), 'T');
    EXPECT_EQ(stream.getPos(), (FilePos {1, 2, 1}));

    stream.seekg(4);
    EXPECT_EQ(stream.get(), '\n');
    EXPECT_EQ(stream.getLine(6), "is");
    EXPECT_EQ(stream.tellg(), 5);

    EXPECT_EQ(stream.getLine(0), "This");
    EXPECT_EQ(stream.tellg(), 5);
}
