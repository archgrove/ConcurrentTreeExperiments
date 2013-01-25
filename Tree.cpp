//
//  Tree.cpp
//  ConcurrentTreeTest
//
//  Created by Adam Wright on 21/01/2013.
//  Copyright (c) 2013 Adam Wright. All rights reserved.
//

#include <thread>
#include <iostream>

#include "Tree.h"

OurMutex Node::rootLock;

void Node::remove(RemoveType type)
{
    switch (type)
    {
        case NoLocks:
            removeNoLocks();
            break;
        case ParentLockOnly:
            removeParentLockOnly();
            break;
        case ParentAndSiblingLocks:
            removeParentAndSiblingLocks();
            break;
        case SiblingLocksBackoff:
            removeSiblingLocksBackoff();
            break;
    }
    
}

void Node::removeParentAndSiblingLocks()
{
    /*
    // Take the parent lock (or the root lock if we have no parent)
    OurMutex *lockingLock;
    if (parent == nullptr)
        lockingLock = &rootLock;
    else
        lockingLock = &parent->parentLock;
    lockingLock->lock();
    
    // If we are the first element of the parent, change it's first child whilst we hold the lock
    removeFixupParent();
    
    // Now handle siblings
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
    
    // Unhook us from the sibling chain
    removeFixupSibs();
    
    if (left != nullptr)
    {
        leftLock.lock();
        left->rightLock.unlock();
        leftLock.unlock();
    }
    
    if (right != nullptr)
    {
        right->leftLock.unlock();
        rightLock.unlock();
    }
    
    removeFixupSelf();
     */
}

#if FOUR_LOCK

inline void Node::lockLeft()
{
    bool lockFailed = true;
    
    while (lockFailed)
    {
        leftLock.lock();

        if (left != nullptr)
        {
            // Lock my left sibling
            lockFailed = !left->rightLock.try_lock();            
        }
        else if (parent != nullptr)
        {
            // No left, so lock my parents first child
            lockFailed = !parent->firstChildLock.try_lock();
        }
        else
        {
            // No left nor parent, so no need to lock anything
            lockFailed = false;
        }
        
        // Backoff if I couldn't lock
        if (lockFailed)
        {
            leftLock.unlock();
        }
    }
}

inline void Node::lockRight()
{
    rightLock.lock();
    
    if (right != nullptr)
    {
        right->leftLock.lock();
    }
    else if (parent != nullptr)
    {
        parent->lastChildLock.lock();
    }
}

inline void Node::unlockLeft()
{
    if (left != nullptr)
    {
        left->rightLock.unlock();
    }
    else if (parent != nullptr)
    {
        parent->firstChildLock.unlock();
    }
    
    leftLock.unlock();
}

inline void Node::unlockRight()
{
    if (right != nullptr)
    {
        right->leftLock.unlock();
    }
    else if (parent)
    {
        parent->lastChildLock.unlock();
    }
    
    rightLock.unlock();
}


void Node::removeSiblingLocksBackoff()
{
    lockLeft();
    lockRight();
    
    removeFixupParent();
    removeFixupSibs();
    
    unlockRight();
    unlockLeft();
    removeFixupSelf();
    
}


#else

inline void Node::lockAll()
{
    bool lockFailed = true;
    
    while (lockFailed)
    {
        //selfLock.lock();
        
        OurMutex *leftLock = nullptr;
        OurMutex *rightLock = nullptr;
        
        if (left != nullptr)
        {
            leftLock = &left->selfLock;
        }
        else if (parent != nullptr)
        {
            leftLock = &parent->selfLock;
        }
        
        if (right != nullptr)
        {
            // Lock my left sibling
            rightLock = &right->selfLock;
        }
        else if (parent != nullptr && left != nullptr)
        {
            // No left, so lock my parents first child
            rightLock = &parent->selfLock;
        }
        
        lockFailed = !leftLock->try_lock();
        
        if (lockFailed)
        {
            selfLock.unlock();
            continue;
        }
        
        if (rightLock != nullptr)
            lockFailed = !rightLock->try_lock();
        
        if (lockFailed)
        {
            leftLock->unlock();
            selfLock.unlock();
        }
    }
}

inline void Node::unlockAll()
{
    if (left != nullptr)
    {
        left->selfLock.unlock();
    }
    else if (parent != nullptr)
    {
        parent->selfLock.unlock();
    }
    
    if (right != nullptr)
    {
        right->selfLock.unlock();
    }
    else if (parent != nullptr && left != nullptr)
    {
        parent->selfLock.unlock();
    }

    selfLock.unlock();
}

void Node::removeSiblingLocksBackoff()
{
    lockAll();
    
    removeFixupParent();
    removeFixupSibs();
    
    unlockAll();
    removeFixupSelf();
}


#endif

void Node::removeNoLocks()
{
    removeFixupParent();
    removeFixupSibs();
    removeFixupSelf();
}

void Node::removeParentLockOnly()
{
    /*
    OurMutex *lockingLock;
    if (parent == nullptr)
        lockingLock = &rootLock;
    else
        lockingLock = &parent->parentLock;
    lockingLock->lock();
    
    removeFixupParent();
    removeFixupSibs();
    
    lockingLock->unlock();
    
    removeFixupSelf();
     */
}

void Node::removeFixupParent()
{
    if (left == nullptr && parent != nullptr)
        parent->firstChild = this->right;
    // Similarly, the last child
    if (right != nullptr && parent != nullptr)
        parent->lastChild = this->left;
}

void Node::removeFixupSibs()
{
    if (left != nullptr)
        left->right = right;
    if (right != nullptr)
        right->left = left;
}

void Node::removeFixupSelf()
{
    parent = nullptr;
    left = nullptr;
    right = nullptr;
}


void Node::appendChild(Node *child)
{
    //parentLock.lock();
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
    //parentLock.unlock();
}

int Node::getData()
{
    return data;
}