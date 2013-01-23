//
//  Tree.cpp
//  ConcurrentTreeTest
//
//  Created by Adam Wright on 21/01/2013.
//  Copyright (c) 2013 Adam Wright. All rights reserved.
//

#include <thread>

#include "Tree.h"

std::mutex Node::rootLock;

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
    // Take the parent lock (or the root lock if we have no parent)
    std::mutex *lockingLock;
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
        left->rightLock.unlock();
        leftLock.unlock();
    }
    
    if (right != nullptr)
    {
        right->leftLock.unlock();
        rightLock.unlock();
    }
    
    removeFixupSelf();
}

void Node::removeSiblingLocksBackoff()
{
    // If we are the first element of the parent, change it's first child whilst we hold the lock
    if (parent != nullptr)
    {
        parent->parentLock.lock();
        
        removeFixupParent();
        
        parent->parentLock.unlock();
    }
    
    // Now handle siblings

    // Take the left and right locks
    // Release the parent (locking) lock
    if (left != nullptr)
    {
        while (true)
        {
            leftLock.lock();
            if (left->rightLock.try_lock())
                break;
            leftLock.unlock();
        }
    }
    
    if (right != nullptr)
    {
        while (true)
        {
            rightLock.lock();
            if (right->leftLock.try_lock())
                break;
            rightLock.unlock();
        }
    }
        
    // Unhook us from the sibling chain
    removeFixupSibs();

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
    
    removeFixupSelf();
}

void Node::removeNoLocks()
{
    removeFixupParent();
    removeFixupSibs();
    removeFixupSelf();
}

void Node::removeParentLockOnly()
{
    std::mutex *lockingLock;
    if (parent == nullptr)
        lockingLock = &rootLock;
    else
        lockingLock = &parent->parentLock;
    lockingLock->lock();
    
    removeFixupParent();
    removeFixupSibs();
    
    lockingLock->unlock();
    
    removeFixupSelf();
}

void Node::removeFixupParent()
{
    if (parent != nullptr && parent->firstChild == this)
        parent->firstChild = this->right;
    // Similarly, the last child
    if (parent != nullptr && parent->lastChild == this)
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