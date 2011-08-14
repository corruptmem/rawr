#include <iostream>
#include <math.h>

#include "GammaLookupTree.h"

GammaLookupTree::GammaLookupTree(double gammaf, int entries) {
    _leaves = new TreeNode[(entries<<1)-1];
    _entries = entries;
    int _cur = 0;
    
    for(int i = 0; i<entries; i++) {
        _leaves[_cur].val = ((double)i)*(1.0/((double)entries-1));
        _leaves[_cur].key = pow(_leaves[_cur].val, gammaf);
        _leaves[_cur].has_val = true;
        _cur++;
    }
    
    int children_count = entries;
    TreeNode* children = _leaves;
    
    while(true) {
        int this_count = children_count/2;
        // stuff
        
        for(int i = 0; i<this_count; i++) {
            TreeNode* c1 = &children[2*i];
            TreeNode* c2 = &children[2*i+1];
            
            double midpoint = (c1->key + c2->key)/2;
            
            _leaves[_cur].key = midpoint;
            _leaves[_cur].child_a = c1;
            _leaves[_cur].child_b = c2;
            _leaves[_cur].has_val = false;
            
            _cur++;
        }
        
        children += children_count;
        children_count = this_count;
        
        // 
        if(this_count == 1) {
            break;
        }
    }
    
    _root = &_leaves[_cur-1];
}

GammaLookupTree::~GammaLookupTree() {
    delete _leaves;
}

double GammaLookupTree::get(double index) {
    TreeNode* node = _root;
    while(node->has_val == false) {
        if(index >= node->key) {
            node = node->child_b;
        } else {
            node = node->child_a;
        }
    }
    
    return node->val;
}