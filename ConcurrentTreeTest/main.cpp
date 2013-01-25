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
const int threadCount = 4;
const uint64_t nodeCount = 1000000;

uint64_t runTest(std::function<void (std::vector<Node *>&)> removeFunc)
{
    uint64_t totalTime = 0;
    // Create nonEdgeNodes + 2 nodes underneath a fixed root node
    for (int i = 0; i < timesToRun; i++)
    {
        // Setup
        Node rootNode;
        std::vector<Node *> sibNodes(nodeCount);
        
        for (uint64_t i = 0; i < nodeCount; i++)
        {
            sibNodes[i] = new Node();
            rootNode.appendChild(sibNodes[i]);
        }
        
        uint64_t start = mach_absolute_time();        
        removeFunc(sibNodes);
        uint64_t end = mach_absolute_time();
        totalTime += (end - start);
        
        // Ensure that every node (apart from the first and last) has no parent
        if (!std::all_of(sibNodes.begin(), sibNodes.end(), [](Node *node){ return node->parent == nullptr; }))
        {
            std::cout << "  Something went very wrong!" << std::endl;
        }
        
        // Cleanup heap memory
        for (uint64_t i = 0; i < nodeCount; i++)
        {
            delete sibNodes[i];
        }
    }
    
    return totalTime;
}

int main(int argc, const char * argv[])
{
#ifdef DEBUG
    std::cout << "I'm a DEBUG build!" << std::endl;
#else
    std::cout << "I'm a RELEASE build!" << std::endl;
#endif
    
    static_assert(nodeCount % threadCount == 0, "Edge nodes must divide thread-count");
    
    // Show the hardware concurrency level
    std::cout << "I'll use " << threadCount << " threads with " << nodeCount << " central nodes, times averaged over " << timesToRun << " runs" << std::endl;
    std::cout << "Hardware has " << std::thread::hardware_concurrency() << " threads" << std::endl;

    // The threaded implementations start threadCount threads, each handling threadCount / nonEdgeNodes nodes in a linear partition
    auto computeFunc = [](RemoveType removeType) {
        return [=](std::vector<Node*> &sibNodes) {
            std::thread threads[threadCount];
            const uint64_t partitionSize = nodeCount / threadCount;
            
            for (int t = 0; t < threadCount; t++)
            {
                threads[t] = std::thread([=, &sibNodes] {
                    for (uint64_t offset = 0; offset < partitionSize; offset++)
                    {
                        sibNodes[partitionSize * t + offset]->remove(removeType);
                    }
                });
            }
            
            for (int t = 0; t < threadCount; t++)
            {
                threads[t].join();
            }
        };
    };

    // The non-threaded implementation just runs through start to end
    auto computeFuncLinear = [](std::vector<Node*>& sibNodes) {
       for (uint64_t offset = 0; offset < nodeCount; offset++)
       {
           sibNodes[offset]->remove(NoLocks);
           //sibNodes[offset]->getData();
       }
    };
    
    uint64_t totalTime;
    uint64_t linearTime;
    
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "Unthreaded implementation" << std::endl;
    linearTime = runTest(computeFuncLinear);
    std::cout << "  Ticks taken: " << linearTime / timesToRun << " over " << timesToRun << " runs" << std::endl;
        
//    std::cout << "-----------------------------------------" << std::endl;
//    std::cout << "Parent lock only implementation" << std::endl;
//    totalTime = runTest(computeFunc(ParentLockOnly));
//    std::cout << "  Ticks taken: " << totalTime / timesToRun << " over " << timesToRun << " runs" << std::endl;
//    std::cout << "  That's " << ((double)totalTime / linearTime) * 100 << "% of the linear time" << std::endl;
//    
//    std::cout << "-----------------------------------------" << std::endl;
//    std::cout << "Parent and sibling locks implementation" << std::endl;
//    totalTime = runTest(computeFunc(ParentAndSiblingLocks));
//    std::cout << "  Ticks taken: " << totalTime / timesToRun << " over " << timesToRun << " runs" << std::endl;
//    std::cout << "  That's " << ((double)totalTime / linearTime) * 100 << "% of the linear time" << std::endl;
    
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "Sibling locks plus backoff implementation" << std::endl;
    totalTime = runTest(computeFunc(SiblingLocksBackoff));
    std::cout << "  Ticks taken: " << totalTime / timesToRun << " over " << timesToRun << " runs" << std::endl;
    std::cout << "  That's " << ((double)totalTime / linearTime) * 100 << "% of the linear time" << std::endl;
}

