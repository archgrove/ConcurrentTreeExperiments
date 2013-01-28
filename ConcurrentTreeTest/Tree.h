//
//  Tree.h
//  ConcurrentTreeTest
//
//  Created by Adam Wright on 21/01/2013.
//  Copyright (c) 2013 Adam Wright. All rights reserved.
//

#include <atomic>

#define FOUR_LOCK false

// Data race writes are to locked flag, so no atomic needed
struct SpinLock
{
    std::atomic_flag locked;
    
    SpinLock() : locked(ATOMIC_FLAG_INIT)
    {
    }
    
    void lock()
    {
        while (std::atomic_flag_test_and_set_explicit(&locked, std::memory_order_acq_rel));
    }
    
    void unlock()
    {
        std::atomic_flag_clear_explicit(&locked, std::memory_order_release);
    }
    
    bool try_lock()
    {
        return (std::atomic_flag_test_and_set_explicit(&locked, std::memory_order_acq_rel) == false);
    }
};

typedef SpinLock OurMutex;
//typedef std::mutex OurMutex;


enum RemoveType
{
    NoLocks,
    ParentLockOnly,
    ParentAndSiblingLocks,
    SiblingLocksBackoff
};

struct Node
{
    //private:
    Node *parent;
    Node *left;
    Node *right;
    Node *firstChild;
    Node *lastChild;
    int data;
    
    OurMutex ownedLock;
    OurMutex *refLock;

    static OurMutex rootLock;
    
public:
    Node() : parent(nullptr), left(nullptr), right(nullptr), firstChild(nullptr), lastChild(nullptr), data(0), refLock(nullptr)
    {
    }
    
    void remove(RemoveType type);
    
    void appendChild(Node *child);
    int getData();
    
protected:
    void removeNoLocks();
    void removeSiblingLocksBackoff();
    
    void removeFixupParent();    
    void removeFixupSibs();
    void removeFixupSibsThreaded();
    void removeFixupSelf();
    
#if FOUR_LOCK
    inline void lockLeft();
    inline void lockRight();
    inline void unlockLeft();
    inline void unlockRight();
#else
    inline void lockAll();
    inline void unlockAll();
#endif
};
