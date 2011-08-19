#include <iostream>
#include <fstream>
#include <math.h>
#include <unistd.h>

#include "Cr2Reader.h"
#include "GammaLookupTree.h"

void processMultiImage(std::string images[], int image_count) {
    RawSensel** sensel = new RawSensel*[image_count];
    
    int w = 0;
    int h = 0;
    double min_val = -1;
    double max_val = -1;
    
    for(int i = 0; i<image_count; i++) {
        Cr2Reader parser(images[i].c_str());
        sensel[i] = parser.Process();
        w = parser.get_width();
        h = parser.get_height();
    }
    
    double* px = new double[w*h];
    
    for(int i = 0; i<w*h; i++) {
        double total_prob = 0;
        for(int img = 0; img<image_count; img++) {
            total_prob += sensel[img][i].prob;
        }
        
        if(total_prob == 0) total_prob = 1;
        
        double val = 0;
        for(int img = 0; img<image_count; img++) {
            val += sensel[img][i].val * (sensel[img][i].prob / total_prob);
        }
        
        min_val = (val < min_val || min_val == -1) ? val : min_val;
        max_val = (val > max_val || max_val == -1) ? val : max_val;
        
        px[i] = val;
    }
    std::cout<<"w: "<<w<<std::endl;
    std::cout<<"h: "<<h<<std::endl;
    std::cout<<"max_val: "<<max_val<<std::endl;
    std::cout<<"min_val: "<<min_val<<std::endl;
    
    GammaLookupTree glt(1.0/0.45, 256);
    
    std::ofstream out("/tmp/out.pgm");
    out<<"P5"<<std::endl;
    out<<w<<std::endl;
    out<<h<<std::endl;
    out<<"255"<<std::endl;
    
    for(int i = 0; i<w*h; i++) {
        double val = px[i];
        
        double scaled_val = ((val-min_val)/(max_val-min_val));
        if(scaled_val < 0) scaled_val = 0;
        if(scaled_val > 1) scaled_val = 1;
        
        unsigned int scaled_val_i = glt.get(scaled_val);
        
        out.put((unsigned char)(scaled_val_i & 0xFF));
        
    }
    
    out.close();
}

int main (int argc, const char * argv[])
{
    std::cout << "Start"  << std::endl;
    
    try {
        std::string c[] = {"./_MG_2084.CR2" , "./_MG_2086.CR2", "./_MG_2089.CR2"};
        processMultiImage(c, 3);
    } catch (const char* str) {
        std::cout<<str<<std::endl;
    }
    
    std::cout << "Done"  << std::endl;
    
    return 0;
}

