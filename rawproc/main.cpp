//
//  main.cpp
//  rawproc
//
//  Created by Cameron Harris on 02/08/2011.
//  Copyright 2011 Altis Partners. All rights reserved.
//

#include <iostream>
#include <fstream>

#include "jpeg.h"

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
        
        char* raw_buf = new char[strip_byte_counts];
        file.seekg(strip_offset, std::ios_base::beg);
        file.read(raw_buf, strip_byte_counts);
        
        ljpeg jp((unsigned char*)raw_buf, strip_byte_counts);
        jp.start(0);
        //----------
        
        ushort *rp;
    
        int h = 3516;
        int w = 5344;
        
        int j_h = jp.jhactual.high;
        int j_w = jp.jhactual.wide;
        int j_c = 4;
        
        uint16_t* raw_px = new uint16_t[j_h*j_w*j_c];
        
        int next = 0;
        for(int jrow = 0; jrow<jp.jhactual.high; jrow++) {
            rp = jp.row(jrow);
            for(int px_c = 0; px_c<j_w*j_c; px_c++) {
                raw_px[next++] = rp[px_c];
            }
        }
        jp.end();
        
        
        next = 0;
        uint16_t* pixels = new uint16_t[w*h];
        
        for(int i = 0; i<w*h; i++) pixels[i] = 0;
        
        for(int strip_i = 0; strip_i<slices.num_first_strips+1; strip_i++) {
            for(int y = 0; y<h; y++) {
                for(int x = 0; x<(strip_i==slices.num_first_strips?slices.last_strip_px:slices.first_strip_px); x+=4) {
                    //pixels[y*w+x+slices.first_strip_px*strip_i] = raw_px[j_w*y+x*j_c];
                    pixels[y*w+x+slices.first_strip_px*strip_i] = raw_px[next];
                    pixels[y*w+x+slices.first_strip_px*strip_i+1] = raw_px[next+1];
                    pixels[y*w+x+slices.first_strip_px*strip_i+2] = raw_px[next+2];
                    pixels[y*w+x+slices.first_strip_px*strip_i+3] = raw_px[next+3];
                    
                    next+=4;
                }
            }
        }
        
        double f_number = 8;
        double exposure = 1/500;
        double iso = 100;
        double q = 0.65; // http://en.wikipedia.org/wiki/Film_speed
        
        
        for(int i = 0; i<w*h; i++) {
            
        }
        
        /*
        std::ofstream out("/tmp/out.pgm");
        out<<"P5"<<std::endl;
        out<<w<<std::endl;
        out<<h<<std::endl;
        out<<"65535"<<std::endl;
        
        for(int i = 0; i<w*h; i++) {
            unsigned char c1;
            unsigned char c2;
            
            c2 = (unsigned char)(pixels[i] & 0xFF);
            c1 = (unsigned char)((pixels[i] & 0xFF00)>>8);
            
            out.put(c1);
            out.put(c2);
        }
        
        out.close();*/
        
        
    /*
        std::ofstream out("/tmp/out.pgm");
        out<<"P5"<<std::endl;
        out<<w<<std::endl;
        out<<h<<std::endl;
        out<<"65535"<<std::endl;
        
        for(int i = 0; i<w*h; i++) {
            unsigned char c1;
            unsigned char c2;
            
            c2 = (unsigned char)(pixels[i] & 0xFF);
            c1 = (unsigned char)((pixels[i] & 0xFF00)>>8);
            
            out.put(c1);
            out.put(c2);
        }
        
        out.close();*/
        
        std::cout << "Done"  << std::endl;
    }
};

int main (int argc, const char * argv[])
{
    try {
        const char* file_path = "/Users/cameron/Pictures/Aperture Library.aplibrary/Masters/2011/07/23/20110723-203953/_MG_1846.CR2";
        Cr2Parser parser(file_path);
        parser.Parse();
    } catch (const char* str) {
        std::cout<<str<<std::endl;
    }
    
    return 0;
}

