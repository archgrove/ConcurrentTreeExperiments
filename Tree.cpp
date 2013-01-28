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
        case SiblingLocksBackoff:
            removeSiblingLocksBackoff();
            break;
        default:
            break;
    }
    
}

inline void Node::lockAll()
{
    while (true)
    {
        if (ownedLock.try_lock())
        {
            if (refLock == nullptr || refLock->try_lock())
            {
                break;
            }
            
            ownedLock.unlock();
        }
    }
}

inline void Node::unlockAll()
{
    if (refLock != nullptr)
        refLock->unlock();
    ownedLock.unlock();
}

void Node::removeSiblingLocksBackoff()
{
    lockAll();
    
    removeFixupParent();
    removeFixupSibsThreaded();
    
    unlockAll();
    removeFixupSelf();
}

void Node::removeNoLocks()
{
    removeFixupParent();
    removeFixupSibs();
    removeFixupSelf();
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
    {
        left->right = right;
    }
    
    if (right != nullptr)
    {
        right->left = left;
    }
}

void Node::removeFixupSibsThreaded()
{
    if (left != nullptr)
        left->right = right;
    
    if (right != nullptr)
    {
        right->left = left;
        
        if (left)
        {
            right->refLock = &left->ownedLock;
        }
        else
        {
            right->refLock = nullptr;
        }
    }
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
        child->refLock = nullptr;
    }
    else
    {
        lastChild->right = child;
        child->left = lastChild;
        child->refLock = &lastChild->ownedLock;
        lastChild = child;
    }
    
    child->parent = this;
    
    //parentLock.unlock();
}

int Node::getData()
{
    return data;
}