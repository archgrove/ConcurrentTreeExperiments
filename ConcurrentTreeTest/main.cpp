//
//  main.c
//  ConcurrentTreeTest
//
//  Created by Adam Wright on 21/01/2013.
//  Copyright (c) 2013 Adam Wright. All rights reserved.
//

#include "Options.h"

#include <stdio.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <thread>
#include <vector>
#include <algorithm>

#include "Tree.h"

void averageTimedComputation(std::function<void (void)> func, int times)
{
    uint64_t totalTime = 0;
    
    for (int i = 0; i < times; i++)
    {
        uint64_t start = mach_absolute_time();

        func();

        uint64_t end = mach_absolute_time();
        totalTime += (end - start);
    }
    
    printf("Ticks taken: %zd over %i runs\n", totalTime / times, times);
}

int main(int argc, const char * argv[])
{
    const int timesToRun = 5;
    const int threadCount = 10;
    const uint64_t nonEdgeNodes = 1000000;
    
    static_assert(nonEdgeNodes % threadCount == 0, "Edge nodes must divide thread-count");
    
    // Show the hardware concurrency level
    printf("Hardware will use %d threads\n", std::thread::hardware_concurrency());
    
    // Create nonEdgeNodes + 2 nodes underneath a fixed root node
    Node rootNode;
    std::vector<Node *> sibNodes(nonEdgeNodes + 2);
    
    for (uint64_t i = 0; i < nonEdgeNodes + 2; i++)
    {
        sibNodes[i] = new Node();
        rootNode.appendChild(sibNodes[i]);
    }
    
#if THREADED
    // The threaded implementation starts threadCount threads, each handling threadCount / nonEdgeNodes nodes
    // in a linear partition
    auto computeFunc = [&] {
        std::thread threads[threadCount];
        const uint64_t partitionSize = nonEdgeNodes / threadCount;
        
        for (int t = 0; t < threadCount; t++)
        {
            threads[t] = std::thread([=, &sibNodes] {
                for (uint64_t offset = 0; offset < partitionSize; offset++)
                {
                    sibNodes[partitionSize * t + offset + 1]->remove();
                }
            });
        }
        
        for (int t = 0; t < threadCount; t++)
        {
            threads[t].join();
        }
    };
#else
    // The non-threaded implementation just runs through start to end
    auto computeFunc = [&] {
       for (uint64_t offset = 0; offset < nonEdgeNodes; offset++)
       {
           sibNodes[offset + 1]->remove();
       }
    };
#endif
    
    averageTimedComputation(computeFunc, timesToRun);
    
    // Ensure that every node (apart from the first and last) has no parent
    if (std::all_of(sibNodes.begin() + 1, sibNodes.end() - 1, [](Node *node){ return node->parent == nullptr; }))
    {
        printf("Algorithm worked OK!");
    }
    else
    {
        printf("Something went very wrong");
    }
    
    // Cleanup heap memory
    for (uint64_t i = 0; i < nonEdgeNodes + 2; i++)
    {
        delete sibNodes[i];
    }
}

