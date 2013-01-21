//
//  Tree.h
//  ConcurrentTreeTest
//
//  Created by Adam Wright on 21/01/2013.
//  Copyright (c) 2013 Adam Wright. All rights reserved.
//


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
    
    void remove();
    void appendChild(Node *child);
};
