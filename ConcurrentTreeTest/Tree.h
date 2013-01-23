//
//  Tree.h
//  ConcurrentTreeTest
//
//  Created by Adam Wright on 21/01/2013.
//  Copyright (c) 2013 Adam Wright. All rights reserved.
//

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
    
    std::mutex parentLock;
    std::mutex leftLock;
    std::mutex rightLock;
    static std::mutex rootLock;
    
public:
    Node() : parent(nullptr), left(nullptr), right(nullptr), firstChild(nullptr), lastChild(nullptr)
    {
    }
    
    void remove(RemoveType type);
    
    void appendChild(Node *child);
    
protected:
    void removeNoLocks();
    void removeParentLockOnly();
    void removeParentAndSiblingLocks();
    void removeSiblingLocksBackoff();
    
    void removeFixupParent();    
    void removeFixupSibs();
    void removeFixupSelf();
};
