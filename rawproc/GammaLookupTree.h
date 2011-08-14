#ifndef rawproc_GammaLookupTree_h
#define rawproc_GammaLookupTree_h

class TreeNode {
public:
    double start;
    double split;
    double end;
    
    int val;
    TreeNode* child_a;
    TreeNode* child_b;
    bool has_val;
};

class GammaLookupTree {
private:
    TreeNode* _leaves;
    TreeNode* _root;
    int _entries;
    
public:
    GammaLookupTree(double gammaf, int entries);
    ~GammaLookupTree();
    
    unsigned int get(double index);
};


#endif
