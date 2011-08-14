#include <iostream>
#include <fstream>
#include <math.h>
#include <unistd.h>

#include "Cr2Reader.h"

int main (int argc, const char * argv[])
{
    std::cout << "Start"  << std::endl;
    
    try {
        const char* file_path = "./_MG_2016.CR2";
        //const char* file_path = "/Users/cameron/Pictures/2011_08_09/_MG_2102.CR2";
        
        Cr2Reader parser(file_path);
        RawSensel* pixels = parser.Process();
        
        int w = parser.get_width();
        int h = parser.get_height();
        
        double min_val = parser.get_min_val();
        double max_val = parser.get_max_val();
        
        // create glut
        
        double glut_idx[256];
        double glut_val[256];
        double gammaf = (1.0/0.45);
        for(int i = 0; i < 256; i++) {
            glut_val[i] = ((double)i)*(1.0/256.0);
            glut_idx[i] = pow(glut_val[i], gammaf);
        }
        
        std::ofstream out("/tmp/out.pgm");
        out<<"P5"<<std::endl;
        out<<w<<std::endl;
        out<<h<<std::endl;
        out<<"255"<<std::endl;
        
        for(int i = 0; i<w*h; i++) {
            double val = pixels[i].val;
            
            double scaled_val = ((val-min_val)/(max_val-min_val));
            if(scaled_val < 0) scaled_val = 0;
            if(scaled_val > 1) scaled_val = 1;
            
            // -- gamma --
            
            int j;
            for(j = 0; j<256; j++) {
                if(scaled_val < glut_idx[j]) {
                    break;
                }
            }
            
            double gamma_corrected = glut_val[j];
            unsigned int scaled_val_i = gamma_corrected*255.0;
            
            // -- // gamma -- 
            
            out.put((unsigned char)(scaled_val_i & 0xFF));
         
        }
        
        out.close();

    } catch (const char* str) {
        std::cout<<str<<std::endl;
    }
    
    std::cout << "Done"  << std::endl;
    
    return 0;
}

