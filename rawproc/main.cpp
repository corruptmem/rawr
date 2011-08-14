#include <iostream>
#include <fstream>

#include "BigInteger.hh"
#include "BigIntegerUtils.hh"
#include "jpeg.h"
#include "math.h"

struct Cr2Header {
    uint16_t byte_order;
    uint16_t magic_word;
    uint32_t tiff_offset;
    uint16_t cr2_magic_word;
    uint8_t cr2_major_ver;
    uint8_t cr2_minor_ver;
    uint32_t raw_ifd_offset;
    
public:
    bool is_valid_header() {
        if(byte_order!= 0x4949) 
            return false;
        
        if(magic_word!= 0x002a)
            return false;
        
        if(tiff_offset != 0x10)
            return false;
        
        if(cr2_magic_word != 0x5243)
            return false;
        
        if(cr2_major_ver!=0x2)
            return false;
        
        if(cr2_minor_ver != 0x0)
            return false;
        
        return true;
    }
};

struct Cr2IfdEntry {
    uint16_t tag_id;
    uint16_t tag_type;
    uint32_t number_of_val;
    uint32_t val;
};

struct Cr2IfdHeader {
    uint16_t entries_count;
};

struct Cr2IfdFooter {
    uint32_t next_ifd;
};

struct Cr2Slices {
    uint16_t num_first_strips;
    uint16_t first_strip_px;
    uint16_t last_strip_px;
};

class Cr2Parser {
private:
    const char* _path;
    
public:
    Cr2Parser(const char* path) {
        _path = path;
    }
    
    void Parse() {
        std::ifstream file(_path);
        
        if(!file.is_open()) {
            throw "Error: Could not open file!";
        }
        
        Cr2Header header;
        file.read((char*)&header, sizeof(Cr2Header));
        
        if(!header.is_valid_header()){
            throw "Error: Invalid header!";
        }
        
        file.seekg(header.raw_ifd_offset, std::ios_base::beg);
        
        Cr2IfdHeader ifd_header;
        file.read((char*)&ifd_header, sizeof(Cr2IfdHeader));
        
        uint32_t strip_offset;
        uint32_t strip_byte_counts;
        uint32_t slice_info_offset;
        
        for(int i = 0; i<ifd_header.entries_count; i++) {
            Cr2IfdEntry ifd_entry;
            file.read((char*)&ifd_entry, sizeof(Cr2IfdEntry));
            
            switch(ifd_entry.tag_id) {
                case 0x103:
                    if(ifd_entry.val!=6) throw "Err: Didn't understand compression type 6";
                    break;
                case 0x111:
                    strip_offset = ifd_entry.val;
                    break;
                case 0x117:
                    strip_byte_counts = ifd_entry.val;
                    break;
                case 0xc640:
                    slice_info_offset = ifd_entry.val;
            }
        }
        
        Cr2IfdFooter ifd_footer;
        file.read((char*)&ifd_footer, sizeof(Cr2IfdFooter));
        
        Cr2Slices slices;
        file.seekg(slice_info_offset, std::ios_base::beg);
        file.read((char*)&slices, sizeof(Cr2Slices));
        file.seekg(strip_offset, std::ios_base::beg);
        
        ljpeg jp(file);
        //----------
        
        unsigned short *rp;
    
        int h = 3516;
        int w = 5344;
        
        int j_h = jp.get_height();
        int j_w = jp.get_width();
        int j_c = 4;
        
        double* pixels = new double[w*h];
        
        double exposure = 0.025;
        double aperture = 9;
        double sensitivity = 100;
        double ev = log2(pow(aperture,2))-log2(exposure)-log2(sensitivity/100);
        double kev = pow(2,ev);
        
        int next = 0;
        double min_val = 100000;
        double max_val = 0;
        
        for(int jrow = 0; jrow<j_h; jrow++) {
            rp = jp.row();
            
            for(int px_c = 0; px_c < j_w*j_c; px_c++) {
                int strip_num = next / (h * slices.first_strip_px);
                if(strip_num > 2) {
                    strip_num = 2;
                }
                
                int x, y;
                if(strip_num != slices.num_first_strips) {
                    y = (next / slices.first_strip_px) % h;
                    x = next % slices.first_strip_px + slices.first_strip_px*strip_num;
                } else {
                    int sub_count = slices.num_first_strips * slices.first_strip_px * h;
                    int this_count = next - sub_count;
                    y = this_count / slices.last_strip_px;
                    x = this_count % slices.last_strip_px + slices.first_strip_px*strip_num;
                }
                
                int pxpos = y*w + x;
                
                if(pxpos > w*h || pxpos < 0) {
                    break;
                }
                
                double pxv = rp[px_c];
                double px = pxv-2048 < 1 ? 1 : pxv-2048;
                double val = kev*(px/1000);
                
                if(val < min_val) min_val = val;
                if(val > max_val) max_val = val;
                
                pixels[pxpos] = val;
                
                next++;
            }
        }
        
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
            double val = pixels[i];
            
            double scaled_val = ((val-min_val)/(max_val-min_val));
            if(scaled_val < 0) scaled_val = 0;
            if(scaled_val > 1) scaled_val = 1;
            //double gamma_corrected = pow(scaled_val,0.45);
            //unsigned int scaled_val_i = gamma_corrected*255.0;
            
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
        
       
    }
};

int main (int argc, const char * argv[])
{
    std::cout << "Start"  << std::endl;
    
    try {
        const char* file_path = "/Users/cameron/Pictures/Aperture Library.aplibrary/Masters/2011/08/09/20110809-211100/_MG_2038.CR2";
        //const char* file_path = "/Users/cameron/Pictures/2011_08_09/_MG_2102.CR2";
        
        Cr2Parser parser(file_path);
        parser.Parse();
    } catch (const char* str) {
        std::cout<<str<<std::endl;
    }
    
    std::cout << "Done"  << std::endl;
    
    return 0;
}

