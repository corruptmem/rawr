#include <fstream>
#include <iostream>

int main (int argc, const char * argv[])
{
    try {
        std::ifstream file("/Users/cameron/Desktop/bike.pbm");
        
        char c1, c2;
        int w, h, px;
        
        file >> c1 >> c2 >> w >> h >> px;
        
        if(c1 != 'P' || c2 != '6') throw "Not P6";
        if(px != 255) throw "Not 255";
        
        short int* cfa = new short int[h*w];
        for(int y = 0; y<h; y++) {
            for(int x = 0; x<w; x++) {
                
            }
        }
        
    } catch(const char* c) {
        std::cout<<c<<std::endl;
    } catch(...) {
        std::cout<<"err"<<std::endl;
    }

    
    return 0;
}   

