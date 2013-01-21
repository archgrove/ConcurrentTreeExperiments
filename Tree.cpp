//
//  Tree.cpp
//  ConcurrentTreeTest
//
//  Created by Adam Wright on 21/01/2013.
//  Copyright (c) 2013 Adam Wright. All rights reserved.
//

#include "Options.h"

#include <thread>

#include "Tree.h"

std::mutex Node::rootLock;

void Node::remove()
{
#if THREADED
    // Take the parent lock (or the root lock if we have no parent)
    std::mutex *lockingLock;
    if (parent == nullptr)
        lockingLock = &rootLock;
    else
        lockingLock = &parent->parentLock;
    lockingLock->lock();
#endif
    
    // If we are the first element of the parent, change it's first child whilst we hold the lock
    if (parent != nullptr && parent->firstChild == this)
        parent->firstChild = this->right;
    // Similarly, the last child
    if (parent != nullptr && parent->lastChild == this)
        parent->lastChild = this->left;
    
    // Now handle siblings
#if THREADED && USE_SIB_LOCKS
    // Take the left and right locks
    // Release the parent (locking) lock
    if (left != nullptr)
    {
        leftLock.lock();
        left->rightLock.lock();
    }
    
    if (right != nullptr)
    {
        rightLock.lock();
        right->leftLock.lock();
    }
    
    lockingLock->unlock();
#endif
    
    // Unhook us from the sibling chain
    if (left != nullptr && right != nullptr)
    {
        left->right = right;
        right->left = left;
    }
    // ASSUMPTION: We are in the middle of a list
    
#if THREADED
#if USE_SIB_LOCKS
    if (left != nullptr)
    {
        left->rightLock.unlock();
        leftLock.unlock();
    }
    
    if (right != nullptr)
    {
        right->leftLock.unlock();
        rightLock.unlock();
    }
#else
    // Release the locking lock
    lockingLock->unlock();
#endif
    #endif
    
    parent = nullptr;
    left = nullptr;
    right = nullptr;
}

void Node::appendChild(Node *child)
{
    parentLock.lock();
    if (firstChild == nullptr)
    {
        firstChild = child;
        lastChild = child;
    }
    else
    {
        lastChild->right = child;
        child->left = lastChild;
        lastChild = child;
    }
    
    child->parent = this;
    parentLock.unlock();
}