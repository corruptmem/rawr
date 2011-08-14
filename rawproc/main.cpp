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

struct RawSensel {
    double val;
    double prob;
};

class Cr2Parser {
private:
    const char* _path;
    int _sensor_width;
    int _sensor_height;
    int _sensor_left_border;
    int _sensor_right_border;
    int _sensor_bottom_border;
    int _sensor_top_border;
    
    double _sensitivity;
    double _exposure_time;
    double _f_number;
    
    double _min_val;
    double _max_val;
    
public:
    Cr2Parser(const char* path) {
        _path = path;
    }
    
    int get_width() {
        return _sensor_width;
    }
    
    int get_height() {
        return _sensor_height;
    }
    
    double get_min_val() {
        return _min_val;
    }
    
    double get_max_val() {
        return _max_val;
    }
    
    RawSensel* Parse() {
        std::ifstream file(_path);
        
        if(!file.is_open()) {
            throw "Error: Could not open file!";
        }
        
        Cr2Header header;
        file.read((char*)&header, sizeof(Cr2Header));
        
        if(!header.is_valid_header()){
            throw "Error: Invalid header!";
        }
        
        // -- ifd#0 read 
        Cr2IfdHeader ifd0_header;
        file.read((char*)&ifd0_header, sizeof(Cr2IfdHeader));
        
        uint32_t exif_ptr = 0;
        
        for(int i = 0; i<ifd0_header.entries_count; i++) {
            Cr2IfdEntry ifd_entry;
            file.read((char*)&ifd_entry, sizeof(Cr2IfdEntry));
            switch(ifd_entry.tag_id) {
                case 34665:
                    // exif
                    exif_ptr = ifd_entry.val;
            }
        }
        
        uint32_t makernote_ptr = 0;
        uint32_t fnum_ptr = 0;
        uint32_t shutter_ptr = 0;
        
        if(exif_ptr != 0) {
            file.seekg(exif_ptr, std::ios_base::beg);
            Cr2IfdHeader exif_ifd_header; 
            file.read((char*) &exif_ifd_header, sizeof(Cr2IfdHeader));
            
            for(int i = 0; i<exif_ifd_header.entries_count; i++){
                Cr2IfdEntry exif_ifd;
                file.read((char*) &exif_ifd, sizeof(Cr2IfdEntry));
                
                switch(exif_ifd.tag_id) {
                    case 0x829d:
                        fnum_ptr = exif_ifd.val;
                        break;
                    case 0x8827:
                        _sensitivity = exif_ifd.val;
                        break;
                    case 0x829a:
                        shutter_ptr = exif_ifd.val;
                        break;
                    case 0x927c:
                        makernote_ptr = exif_ifd.val;
                        break;
                }
            }
        }
        
        if(fnum_ptr != 0) {
            file.seekg(fnum_ptr, std::ios_base::beg);
            
            uint32_t a;
            uint32_t b;
            
            file.read((char*)&a, 4);
            file.read((char*)&b, 4);
            
            _f_number = ((double)a)/((double)b);
        }
        
        if(shutter_ptr != 0) {
            file.seekg(shutter_ptr, std::ios_base::beg);
            
            uint32_t a;
            uint32_t b;
            
            file.read((char*)&a, 4);
            file.read((char*)&b, 4);
            
            _exposure_time = ((double)a)/((double)b);
        }
        
        uint32_t sensor_info_ptr = 0;
        int sensor_info_count;
        
        if(makernote_ptr != 0) {
            file.seekg(makernote_ptr, std::ios_base::beg);
            Cr2IfdHeader makernote_ifd_header;
            file.read((char*) &makernote_ifd_header, sizeof(Cr2IfdHeader));
            
            for(int i = 0; i<makernote_ifd_header.entries_count; i++) {
                Cr2IfdEntry makernote_ifd;
                file.read((char*) &makernote_ifd, sizeof(Cr2IfdEntry));
                
                switch(makernote_ifd.tag_id) {
                    case 0x00e0:
                        sensor_info_ptr = makernote_ifd.val;
                        sensor_info_count = makernote_ifd.number_of_val;
                        break;
                }
            }
        }
        
        if(sensor_info_ptr != 0) {
            file.seekg(sensor_info_ptr, std::ios_base::beg);
            uint16_t sensor_info[sensor_info_count];
            
            file.read((char*) &sensor_info, sizeof(uint16_t)*sensor_info_count);
            
            _sensor_width = sensor_info[1];
            _sensor_height = sensor_info[2];
            _sensor_left_border = sensor_info[5];
            _sensor_top_border = sensor_info[6];
            _sensor_right_border = sensor_info[7];
            _sensor_bottom_border = sensor_info[8];
        }
        
        
        // -- / ifd#0
        
        
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
    
        int h = _sensor_height;
        int w = _sensor_width;
        
        int j_h = jp.get_height();
        int j_w = jp.get_width();
        int j_c = 4;
        
        RawSensel* pixels = new RawSensel[w*h];
        
        double exposure = _exposure_time;
        double aperture = _f_number;
        double sensitivity = _sensitivity;
        
        std::cout<<"Sensor Width: "<<w<<std::endl;
        std::cout<<"Sensor Height: "<<h<<std::endl;
        std::cout<<"Exposure: "<<exposure<<std::endl;
        std::cout<<"Aperture: "<<aperture<<std::endl;
        std::cout<<"Sensitivity: "<<sensitivity<<std::endl;
        
        double ev = log2(pow(aperture,2))-log2(exposure)-log2(sensitivity/100);
        double kev = pow(2,ev);
        
        int next = 0;
        _min_val = 100000;
        _max_val = 0;
        
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
                
                if(val < _min_val) _min_val = val;
                if(val > _max_val) _max_val = val;
                
                pixels[pxpos].val = val;
                double base_prob;
                if(val < 2500) {
                    base_prob = 0.2;
                } else if (val < 3200) {
                    base_prob = 0.4;
                } else if (val < 4500) {
                    base_prob = 0.6;
                } else if (val < 5000) {
                    base_prob = 0.8;
                } else if (val > 13000) {
                    base_prob = 0.8;
                } else if (val > 13500) {
                    base_prob = 0.6;
                } else if (val > 14000) {
                    base_prob = 0.4;
                } else if (val > 15000) {
                    base_prob = 0.2;
                } else {
                    base_prob = 0.9;
                }
                
                pixels[pxpos].prob = base_prob / (sensitivity/100);
                
                
                next++;
            }
            
        }
        
        return pixels;
    }
};

int main (int argc, const char * argv[])
{
    std::cout << "Start"  << std::endl;
    
    try {
        const char* file_path = "/Users/cameron/Pictures/Aperture Library.aplibrary/Masters/2011/08/09/20110809-211100/_MG_2038.CR2";
        //const char* file_path = "/Users/cameron/Pictures/2011_08_09/_MG_2102.CR2";
        
        Cr2Parser parser(file_path);
        RawSensel* pixels = parser.Parse();
        
        
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

    } catch (const char* str) {
        std::cout<<str<<std::endl;
    }
    
    std::cout << "Done"  << std::endl;
    
    return 0;
}

