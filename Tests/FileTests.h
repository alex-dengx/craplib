// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __FILE_TESTS_H__
#define __FILE_TESTS_H__

#include "AsyncFile.h"
#include "RunLoop.h"
#include "Data.h"

SUITE(File);

class SimpleReader 
: AsyncFileReader::Delegate
{
private:
    AsyncFileReader reader_;

public:
    SimpleReader()
    : reader_(this, "test.txt")
    { 
        reader_.read(2048);
    }
    
    virtual void onChunk(const AsyncFileReader& fr, const Data& chunk) 
    {
        wLog("Chunk.. %d", chunk.get_size());
        reader_.read(4096); // Read more
    }
    
    virtual void onEndOfFile(const AsyncFileReader& fr)
    {
        wLog("End of file reached.");
    }
    
    virtual void onError(const AsyncFileReader& fr) 
    {
        wLog("ERROR while reading file.");
    }
};

TEST(BasicReading, File) {
    RunLoop rl;    
    SimpleReader reader;
    rl.run();
};

#endif // __FILE_TESTS_H__