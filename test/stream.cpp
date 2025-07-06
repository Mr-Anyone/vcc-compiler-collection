#include "stream.h"
#include <cstdio>
#include <gtest/gtest.h>
#include <fstream>
#include <type_traits>

// Demonstrate some basic assertions.
TEST(StreamTest, BasicTest) {
    FileStream stream ("resource/streamtest.txt");
    char c = stream.get();
    EXPECT_EQ(c, 'T');
    EXPECT_EQ(stream.tellg(), 1);
    EXPECT_EQ(stream.getPos(), (FilePos {1, 2}));


    stream.get(c);
    EXPECT_EQ(c, 'h');
    EXPECT_EQ(stream.good(), true);
    EXPECT_EQ(stream.tellg(), 2);
    EXPECT_EQ(stream.eof(), false);

    // check for no side effect
    EXPECT_EQ(stream.peek(), 'i');
    EXPECT_EQ(stream.peek(), 'i');
    EXPECT_EQ(stream.tellg(), 2);
    EXPECT_EQ(stream.getPos(), (FilePos {1, 3}));

    while(!stream.eof()){
        stream.get();
    }
    EXPECT_EQ(stream.eof(), true);

    for(long loc: stream.getCurrentNewLineBuf()){
        stream.seekg(loc);
        EXPECT_EQ(stream.tellg(), loc);
        EXPECT_EQ(stream.get(), '\n');
    }
}

TEST(StreamTest, NewLinLocTest){
    FileStream stream ("resource/new_line_loc_test.txt");

    // consume until eof
    while(!stream.eof())
        stream.get();

    EXPECT_EQ(stream.getCurrentNewLineBuf().size(), 38);
    for(long loc: stream.getCurrentNewLineBuf()){
        stream.seekg(loc);
        EXPECT_EQ(stream.tellg(), loc);
        EXPECT_EQ(stream.get(), '\n');
    }
}
