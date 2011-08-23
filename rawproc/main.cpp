#include <iostream>
#include <fstream>
#include <math.h>
#include <unistd.h>

#include "Cr2Reader.h"
#include "GammaLookupTree.h"
#include "cl.hpp"

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
    
    float* px = new float[w*h];
    
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
    
    for(int i = 0; i<image_count; i++) {
        delete sensel[i];
    }
    
    delete sensel;
    
    float r_w = 1.7;
    float g1_w = 0.7;
    float g2_w = 0.7;
    float b_w = 0.9;
    
    int r = 2; // 10
    int b = 1; // 01
    int g1 = 3; // 11
    int g2 = 0; // 00
    
    
    float* px2 = new float[w*h*3]();
    // really simple interpolation
    for(int y = 4; y<(h-4); y++) {
        for(int x = 4; x<(w-4); x++) {
            int g11 = (y%2) ^ ((g1&2)>>1);
            int g12 = (x%2) ^ (g1&1);
            int b1 = (y%2) ^ ((b&2)>>1);
            int b2 = (x%2) ^ (b&1);
            int r1 = (y%2) ^ ((r&2)>>1);
            int r2 = (x%2) ^ (r&1);
            
            px2[3*(y*w+x)+0] = px[(y+r1)*w+(x+r2)]*r_w;
            px2[3*(y*w+x)+1] = px[(y+g11)*w+(x+g12)]*g1_w;
            px2[3*(y*w+x)+2] = px[(y+b1)*w+(x+b2)]*b_w;
        }
    }
    
    std::cout<<"w: "<<w<<std::endl;
    std::cout<<"h: "<<h<<std::endl;
    std::cout<<"max_val: "<<max_val<<std::endl;
    std::cout<<"min_val: "<<min_val<<std::endl;
    
    max_val = 50;
    
    GammaLookupTree glt(1.0/0.45, 256);
  //  GammaLookupTree glt(1.0/0.45, 65536);
    
    std::ofstream out("/tmp/out.ppm");
    out<<"P6"<<std::endl;
    out<<w<<std::endl;
    out<<h<<std::endl;
    out<<"255"<<std::endl;
    //out<<"65535"<<std::endl;
    
    for(int i = 0; i<w*h*3; i++) {
        double val = px2[i];
        
        double scaled_val = ((val-min_val)/(max_val-min_val));
        if(scaled_val < 0) scaled_val = 0;
        if(scaled_val > 1) scaled_val = 1;
        
        unsigned int scaled_val_i = glt.get(scaled_val);
        
      //  out.put((unsigned char)((scaled_val_i >> 8) & 0xFF));
        out.put((unsigned char)(scaled_val_i & 0xFF));
        
    }
    
    out.close();
}

int main (int argc, const char * argv[])
{
    std::cout << "Start"  << std::endl;
    
    try {
        //std::string c[] = {"./_MG_2084.CR2" , "./_MG_2086.CR2", "./_MG_2089.CR2"};
        std::string c[] = {"/Users/cameron/Pictures/IMG_0022.CR2" , "/Users/cameron/Pictures/IMG_0021.CR2", "/Users/cameron/Pictures/IMG_0023.CR2"};
        processMultiImage(c, 3);
    } catch (const char* str) {
        std::cout<<str<<std::endl;
    }
    
    std::cout << "Done"  << std::endl;
    
    return 0;
}

