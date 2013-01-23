//
//  main.c
//  ConcurrentTreeTest
//
//  Created by Adam Wright on 21/01/2013.
//  Copyright (c) 2013 Adam Wright. All rights reserved.
//

#include <mach/mach.h>
#include <mach/mach_time.h>

#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>

#include "Tree.h"

const int timesToRun = 5;
const int threadCount = 2;
const uint64_t nonEdgeNodes = 50000;

uint64_t averageTimedComputation(std::function<void (void)> func, int times)
{
    uint64_t totalTime = 0;
    
    for (int i = 0; i < times; i++)
    {
        uint64_t start = mach_absolute_time();
        
        func();
        
        uint64_t end = mach_absolute_time();
        totalTime += (end - start);
    }
    
    return totalTime;
}

uint64_t runTest(std::function<std::function<void()> (std::vector<Node *>&)> removeFunc)
{
    // Create nonEdgeNodes + 2 nodes underneath a fixed root node
    Node rootNode;
    std::vector<Node *> sibNodes(nonEdgeNodes + 2);
    
    for (uint64_t i = 0; i < nonEdgeNodes + 2; i++)
    {
        sibNodes[i] = new Node();
        rootNode.appendChild(sibNodes[i]);
    }
    
    uint64_t totalTime = averageTimedComputation(removeFunc(sibNodes), timesToRun);
    
    // Ensure that every node (apart from the first and last) has no parent
    if (std::all_of(sibNodes.begin() + 1, sibNodes.end() - 1, [](Node *node){ return node->parent == nullptr; }))
    {
        std::cout << "  Algorithm worked OK!" << std::endl;
    }
    else
    {
        std::cout << "  Something went very wrong!" << std::endl;
    }
    
    // Cleanup heap memory
    for (uint64_t i = 0; i < nonEdgeNodes + 2; i++)
    {
        delete sibNodes[i];
    }
    
    return totalTime;
}

int main(int argc, const char * argv[])
{
    static_assert(nonEdgeNodes % threadCount == 0, "Edge nodes must divide thread-count");
    
    // Show the hardware concurrency level
    std::cout << "Hardware will use " << std::thread::hardware_concurrency() << " threads" << std::endl;

    // The threaded implementations start threadCount threads, each handling threadCount / nonEdgeNodes nodes in a linear partition
    auto computeFunc = [](RemoveType removeType) {
        return [=](std::vector<Node*> &sibNodes) {
            return [&] {
                std::thread threads[threadCount];
                const uint64_t partitionSize = nonEdgeNodes / threadCount;
                
                for (int t = 0; t < threadCount; t++)
                {
                    threads[t] = std::thread([=, &sibNodes] {
                        for (uint64_t offset = 0; offset < partitionSize; offset++)
                        {
                            sibNodes[partitionSize * t + offset + 1]->remove(removeType);
                        }
                    });
                }
                
                for (int t = 0; t < threadCount; t++)
                {
                    threads[t].join();
                }
            };
        };
    };

    // The non-threaded implementation just runs through start to end
    auto computeFuncLinear = [](std::vector<Node*>& sibNodes) {
        return [&] {
           for (uint64_t offset = 0; offset < nonEdgeNodes; offset++)
           {
               sibNodes[offset + 1]->remove(NoLocks);
           }
        };
    };
    
    uint64_t totalTime;
    uint64_t linearTime;
    
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "Unthreaded implementation" << std::endl;
    linearTime = runTest(computeFuncLinear);
    std::cout << "  Ticks taken: " << linearTime / timesToRun << " over " << timesToRun << " runs" << std::endl;
        
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "Parent lock only implementation" << std::endl;
    totalTime = runTest(computeFunc(ParentLockOnly));
    std::cout << "  Ticks taken: " << totalTime / timesToRun << " over " << timesToRun << " runs" << std::endl;
    std::cout << "That's " << ((double)totalTime / linearTime) * 100 << "% of the linear time" << std::endl;
    
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "Parent and sibling locks implementation" << std::endl;
    totalTime = runTest(computeFunc(ParentAndSiblingLocks));
    std::cout << "  Ticks taken: " << totalTime / timesToRun << " over " << timesToRun << " runs" << std::endl;
    std::cout << "That's " << ((double)totalTime / linearTime) * 100 << "% of the linear time" << std::endl;
    
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "Sibling locks plus backoff implementation" << std::endl;
    totalTime = runTest(computeFunc(SiblingLocksBackoff));
    std::cout << "  Ticks taken: " << totalTime / timesToRun << " over " << timesToRun << " runs" << std::endl;
    std::cout << "That's " << ((double)totalTime / linearTime) * 100 << "% of the linear time" << std::endl;
}

